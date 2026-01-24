#include "playlist-sync-service.h"

#include "api/api-manager.h"
#include "services/service-types.h"

#include <warp/log.h>
#include <warp/log-utils.h>
#include <warp/utils.h>

#include <algorithm>
#include <ranges>
#include <unordered_set>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("Playlist Sync");
      constexpr auto PLAYLIST_UPDATE_WAIT_TIME(std::chrono::milliseconds(100));
   }

   PlaylistSyncService::PlaylistSyncService(const PlaylistSyncConfig& config,
                                            std::shared_ptr<ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_PLAYLIST_SYNC, apiManager, config.cron)
      , timeForEmbyUpdateSec_(config.time_for_emby_to_update_seconds)
      , timeBetweenSyncsSec_(config.time_between_syncs_seconds)
   {
      Init(config);
   }

   void PlaylistSyncService::Init(const PlaylistSyncConfig& config)
   {
      for (const auto& plexCollection : config.plex_collection_sync)
      {
         if (auto plexApi = GetApiManager()->GetPlexApi(plexCollection.server);
             plexApi != nullptr)
         {
            if (plexApi->GetValid() && !plexApi->GetCollectionValid(plexCollection.library, plexCollection.collection_name))
            {
               LogWarning("{} {} {} not found on server",
                          plexApi->GetPrettyName(),
                          warp::GetTag("library", plexCollection.library),
                          warp::GetTag("collection", plexCollection.collection_name));
               continue;
            }

            PlaylistPlexCollection collection;
            for (const auto& embyServerName : plexCollection.target_emby_servers)
            {
               if (auto* embyApi{GetApiManager()->GetEmbyApi(embyServerName.server)}; embyApi)
               {
                  collection.target_emby_servers.emplace_back(embyServerName);
               }
               else
               {
                  LogWarning("{} api not found for {} {}",
                             warp::GetServerName(warp::GetFormattedEmby(), embyServerName.server),
                             plexApi->GetPrettyName(),
                             warp::GetTag("collection", plexCollection.collection_name));
               }
            }

            // If there are no emby servers to sync do add to collections
            if (!collection.target_emby_servers.empty())
            {
               collection.collection_name = plexCollection.collection_name;
               collection.server = plexCollection.server;
               collection.library = plexCollection.library;
               plexCollections_.emplace_back(plexCollection);
            }
            else
            {
               LogWarning("{} {} {} no emby servers to sync ... skipping",
                          plexApi->GetPrettyName(),
                          warp::GetTag("library", plexCollection.library),
                          warp::GetTag("collection", plexCollection.collection_name));
            }
         }
         else
         {
            LogWarning("No {} found with {} ... Skipping",
                       warp::GetFormattedPlex(),
                       warp::GetTag("server_name", plexCollection.server));
         }
      }
   }

   std::pair<size_t, size_t> PlaylistSyncService::AddRemoveEmbyPlaylistItems(EmbyApi* embyApi,
                                                                             const EmbyPlaylist& currentPlaylist,
                                                                             const std::vector<std::string>& updatedPlaylistIds)
   {
      std::unordered_set<std::string_view> currentItemIds;
      currentItemIds.reserve(currentPlaylist.items.size());
      for (const auto& item : currentPlaylist.items)
      {
         currentItemIds.emplace(item.id);
      }

      std::vector<std::string> addIds;
      addIds.reserve(updatedPlaylistIds.size());
      for (const auto& targetId : updatedPlaylistIds)
      {
         if (!currentItemIds.contains(targetId))
         {
            addIds.emplace_back(targetId);
         }
      }

      std::unordered_set<std::string_view> targetIdSet;
      targetIdSet.reserve(updatedPlaylistIds.size());
      for (const auto& id : updatedPlaylistIds)
      {
         targetIdSet.emplace(id);
      }

      std::vector<std::string> deleteIds;
      for (const auto& item : currentPlaylist.items)
      {
         if (!targetIdSet.contains(item.id))
         {
            // IMPORTANT: Remove using the PlaylistEntryId, not the Media ID
            deleteIds.emplace_back(item.playlistId);
         }
      }

      auto logPlaylistWarning = [this, &embyApi, &currentPlaylist](bool addErr, const std::vector<std::string>& ids) {
         LogWarning("{} failed to {} {} to {}",
                    embyApi->GetPrettyName(),
                    addErr ? "add" : "remove",
                    warp::GetTag("item_count", ids.size()),
                    warp::GetTag("playlist", currentPlaylist.name));
      };

      // API Calls
      if (!addIds.empty() && !embyApi->AddPlaylistItems(currentPlaylist.id, addIds))
      {
         logPlaylistWarning(true, addIds);
      }

      if (!deleteIds.empty() && !embyApi->RemovePlaylistItems(currentPlaylist.id, deleteIds))
      {
         logPlaylistWarning(false, deleteIds);
      }

      return {addIds.size(), deleteIds.size()};
   }

   void PlaylistSyncService::UpdateEmbyPlaylist(PlexApi* plexApi,
                                                EmbyApi* embyApi,
                                                EmbyPlaylist currentPlaylist,
                                                const std::vector<std::string>& correctIds)
   {
      auto [added, removed] = AddRemoveEmbyPlaylistItems(embyApi, currentPlaylist, correctIds);

      if (added > 0 || removed > 0)
      {
         std::this_thread::sleep_for(std::chrono::seconds(timeForEmbyUpdateSec_));
         auto updatedPlaylist = embyApi->GetPlaylist(currentPlaylist.name); // Re-fetch ONCE after structural changes
         if (updatedPlaylist)
         {
            currentPlaylist = std::move(*updatedPlaylist);
         }
         else
         {
            LogWarning("{} failed to retrieve {} on update",
                    embyApi->GetPrettyName(),
                    warp::GetTag("playlist", currentPlaylist.name));
            return;
         }
      }

      if (currentPlaylist.items.size() != correctIds.size())
      {
         LogWarning("{} sync {} {} playlist updated failed. Playlist length should be {} but {}",
                    embyApi->GetPrettyName(),
                    plexApi->GetPrettyName(),
                    warp::GetTag("collection", currentPlaylist.name),
                    warp::GetTag("length", correctIds.size()),
                    warp::GetTag("reported_length", currentPlaylist.items.size()));
         return;
      }

      struct MoveTracker
      {
         std::string_view id;
         std::string_view pId;
      };
      std::vector<MoveTracker> virtualItems;
      virtualItems.reserve(currentPlaylist.items.size());
      for (const auto& item : currentPlaylist.items) virtualItems.push_back({item.id, item.playlistId});

      bool orderChanged = false;
      for (uint32_t i = 0; i < correctIds.size(); ++i)
      {
         // If already correct, skip search
         if (virtualItems[i].id == correctIds[i]) continue;

         auto it = std::find_if(virtualItems.begin() + i, virtualItems.end(),
                                [&](const auto& vt) { return vt.id == correctIds[i]; });

         if (it != virtualItems.end())
         {
            if (embyApi->MovePlaylistItem(currentPlaylist.id, it->pId, i))
            {
               auto itemToMove = *it;
               virtualItems.erase(it);
               virtualItems.insert(virtualItems.begin() + i, itemToMove);
               orderChanged = true;

               std::this_thread::sleep_for(PLAYLIST_UPDATE_WAIT_TIME);
            }
         }
      }

      if (orderChanged || added > 0 || removed > 0)
      {
         LogInfo("Syncing {} {} to {} {} {} {}",
                 plexApi->GetPrettyName(),
                 warp::GetTag("collection", currentPlaylist.name),
                 embyApi->GetPrettyName(),
                 warp::GetTag("added", added),
                 warp::GetTag("removed", removed),
                 warp::GetTag("reordered", orderChanged ? "true" : "false"));
      }
   }

   void PlaylistSyncService::SyncEmbyPlaylist(PlexApi* plexApi, EmbyApi* embyApi, const PlexCollection& plexCollection)
   {
      if (embyApi->GetPathMapEmpty() && !plexCollection.items.empty())
      {
         LogWarning("{} path map is empty. {} {} can not be synced.",
                    embyApi->GetPrettyName(),
                    plexApi->GetPrettyName(),
                    warp::GetTag("collection", plexCollection.name));
         return;
      }

      std::vector<std::string> updatedPlaylistIds;
      for (auto& item : plexCollection.items)
      {
         bool foundItem{false};
         for (auto& path : item.paths)
         {
            if (auto id = embyApi->GetIdFromPathMap(path))
            {
               foundItem = true;
               updatedPlaylistIds.emplace_back(std::move(*id));
               break;
            }
         }

         if (!foundItem)
         {
            LogWarning("{} sync {} {} {} not found",
                       embyApi->GetPrettyName(),
                       plexApi->GetPrettyName(),
                       warp::GetTag("collection", plexCollection.name),
                       warp::GetTag("item", item.title));
         }
      }

      if (auto currentEmbyPlaylist = embyApi->GetPlaylist(plexCollection.name))
      {
         UpdateEmbyPlaylist(plexApi, embyApi, std::move(*currentEmbyPlaylist), updatedPlaylistIds);
      }
      else
      {
         embyApi->CreatePlaylist(plexCollection.name, updatedPlaylistIds);

         LogInfo("Creating {} {} on {}",
                 plexApi->GetPrettyName(),
                 warp::GetTag("collection", plexCollection.name),
                 embyApi->GetPrettyName());
      }
   }

   void PlaylistSyncService::SyncPlexCollection(PlexApi* plexApi, EmbyApi* embyApi, const PlaylistPlexCollection& collection)
   {
      auto plexCollection = plexApi->GetCollection(collection.library, collection.collection_name);
      if (plexCollection)
      {
         SyncEmbyPlaylist(plexApi, embyApi, *plexCollection);
      }
   }

   void PlaylistSyncService::Run()
   {
      for (auto& plexCollection : plexCollections_)
      {
         if (auto* plexApi{GetApiManager()->GetPlexApi(plexCollection.server)};
             plexApi->GetValid())
         {
            for (const auto& embyServer : plexCollection.target_emby_servers)
            {
               if (auto* embyApi{GetApiManager()->GetEmbyApi(embyServer.server)};
                   embyApi->GetValid())
               {
                  // Sync this plex collection with the emby server
                  SyncPlexCollection(plexApi, embyApi, plexCollection);
               }
            }
         }
      }
   }
}