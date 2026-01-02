#include "playlist-sync-service.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

#include <algorithm>
#include <format>
#include <ranges>

namespace loomis
{
   PlaylistSyncService::PlaylistSyncService(const PlaylistSyncConfig& config,
                                            std::shared_ptr<ApiManager> apiManager)
      : ServiceBase("Playlist Sync", utils::ANSI_CODE_SERVICE_PLAYLIST_SYNC, apiManager, config.cron)
      , timeForEmbyUpdateSec_(config.timeForEmbyUpdateSec)
      , timeBetweenSyncsSec_(config.timeBetweenSyncSec)
   {
      Init(config);
   }

   void PlaylistSyncService::Init(const PlaylistSyncConfig& config)
   {
      for (const auto& plexCollection : config.plexCollections)
      {
         auto plexApi{GetApiManager()->GetPlexApi(plexCollection.server)};
         if (plexApi != nullptr)
         {
            if (plexApi->GetValid() && !plexApi->GetCollectionValid(plexCollection.library, plexCollection.collectionName))
            {
               LogWarning(std::format("{} {} {} not found on server",
                                      utils::GetServerName(utils::GetFormattedPlex(), plexCollection.server),
                                      utils::GetTag("library", plexCollection.library),
                                      utils::GetTag("collection", plexCollection.collectionName)));
               continue;
            }

            PlaylistPlexCollection collection;
            for (const auto& embyServerName : plexCollection.embyServers)
            {
               auto* embyApi{GetApiManager()->GetEmbyApi(embyServerName)};
               if (embyApi)
               {
                  collection.embyServers.emplace_back(embyServerName);
               }
               else
               {
                  LogWarning(std::format("{} api not found for {} {}",
                                         utils::GetServerName(utils::GetFormattedEmby(), embyServerName),
                                         utils::GetServerName(utils::GetFormattedPlex(), plexCollection.server),
                                         utils::GetTag("collection", plexCollection.collectionName)));
               }
            }

            // If there are no emby servers to sync do add to collections
            if (!collection.embyServers.empty())
            {
               collection.collectionName = plexCollection.collectionName;
               collection.server = plexCollection.server;
               collection.library = plexCollection.library;
               plexCollections_.emplace_back(plexCollection);
            }
            else
            {
               LogWarning(std::format("{} {} {} no emby servers to sync ... skipping",
                                      utils::GetServerName(utils::GetFormattedPlex(), plexCollection.server),
                                      utils::GetTag("library", plexCollection.library),
                                      utils::GetTag("collection", plexCollection.collectionName)));
            }
         }
         else
         {
            LogWarning(std::format("No {} found with {} ... Skipping", utils::GetFormattedPlex(), utils::GetTag("server_name", plexCollection.server)));
         }
      }
   }

   std::pair<size_t, size_t> PlaylistSyncService::AddRemoveEmbyPlaylistItems(EmbyApi* embyApi,
                                                                             const EmbyPlaylist& currentPlaylist,
                                                                             const std::vector<std::string>& updatedPlaylistIds)
   {
      std::vector<std::string> addIds;
      for (const auto& targetId : updatedPlaylistIds)
      {
         bool doesNotExist = std::ranges::none_of(currentPlaylist.items,
             [&targetId](const auto& item) { return item.id == targetId; });

         if (doesNotExist) addIds.push_back(targetId);
      }

      std::vector<std::string> deleteIds;
      for (const auto& item : currentPlaylist.items)
      {
         bool doesNotExist = std::ranges::none_of(updatedPlaylistIds,
             [&item](const auto& targetId) { return item.id == targetId; });

         if (doesNotExist) deleteIds.push_back(item.playlistId);
      }

      auto logPlaylistWarning = [this, &embyApi, &currentPlaylist](bool addErr, const std::vector<std::string>& ids) {
         LogWarning(std::format("{} failed to {} {} to {}",
                                utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                addErr ? "add" : "remove",
                                utils::GetTag("item_count", ids.size()),
                                utils::GetTag("playlist", currentPlaylist.name)));
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

   bool PlaylistSyncService::UpdateToCorrectLocation(EmbyApi* embyApi, const EmbyPlaylist& currentPlaylist, std::string_view id, uint32_t correctIndex)
   {
      bool locationChanged{false};

      for (int32_t i = 0; i < currentPlaylist.items.size(); ++i)
      {
         const auto& item{currentPlaylist.items[i]};
         if (id == item.id)
         {
            if (i != correctIndex)
            {
               if (embyApi->MovePlaylistItem(currentPlaylist.id, item.playlistId, correctIndex))
               {
                  // Need to give time for the server to update
                  std::this_thread::sleep_for(std::chrono::seconds(timeBetweenSyncsSec_));

                  LogTrace(std::format("{} {} moving {} {} {}",
                                       utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                       utils::GetTag("playlist", currentPlaylist.name),
                                       utils::GetTag("item", item.name),
                                       utils::GetTag("from", std::to_string(i + 1)),
                                       utils::GetTag("to", std::to_string(correctIndex + 1))));

                  locationChanged = true;
               }
               else
               {
                  LogWarning(std::format("{} failed {} moving {} to {}",
                                         utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                         utils::GetTag("playlist", currentPlaylist.name),
                                         utils::GetTag("item", item.name),
                                         utils::GetTag("index", std::to_string(correctIndex + 1))));
               }
            }
            break;
         }
      }

      return locationChanged;
   }

   void PlaylistSyncService::UpdateEmbyPlaylist(PlexApi* plexApi,
                                                EmbyApi* embyApi,
                                                std::string_view playlistName,
                                                const std::vector<std::string>& correctIds)
   {
      auto currentPlaylist = embyApi->GetPlaylist(playlistName);
      if (!currentPlaylist) return;

      auto [added, removed] = AddRemoveEmbyPlaylistItems(embyApi, *currentPlaylist, correctIds);

      if (added > 0 || removed > 0)
      {
         std::this_thread::sleep_for(std::chrono::seconds(timeForEmbyUpdateSec_));
         currentPlaylist = embyApi->GetPlaylist(playlistName); // Re-fetch ONCE after structural changes
      }

      if (!currentPlaylist)
      {
         LogWarning(std::format("{} failed to retrieve {} on update",
                                utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                utils::GetTag("playlist", playlistName)));
         return;
      }

      if (currentPlaylist->items.size() != correctIds.size())
      {
         LogWarning(std::format("{} sync {} {} playlist updated failed. Playlist length should be {} but {}",
                                utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                utils::GetServerName(utils::GetFormattedPlex(), plexApi->GetName()),
                                utils::GetTag("collection", playlistName),
                                utils::GetTag("length", correctIds.size()),
                                utils::GetTag("reported_length", currentPlaylist->items.size())));
         return;
      }

      // Use a lightweight tracker to avoid copying heavy strings
      struct MoveTracker
      {
         std::string_view id; std::string_view pId;
      };
      std::vector<MoveTracker> virtualItems;
      virtualItems.reserve(currentPlaylist->items.size());
      for (const auto& item : currentPlaylist->items) virtualItems.push_back({item.id, item.playlistId});

      bool orderChanged = false;
      for (uint32_t i = 0; i < correctIds.size(); ++i)
      {
         // If already correct, skip search
         if (virtualItems[i].id == correctIds[i]) continue;

         // Efficiency gain: Search from current position 'i'
         auto it = std::find_if(virtualItems.begin() + i, virtualItems.end(),
                                [&](const auto& vt) { return vt.id == correctIds[i]; });

         if (it != virtualItems.end())
         {
            if (embyApi->MovePlaylistItem(currentPlaylist->id, it->pId, i))
            {
               auto itemToMove = *it;
               virtualItems.erase(it);
               virtualItems.insert(virtualItems.begin() + i, itemToMove);
               orderChanged = true;

               std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
         }
      }

      if (orderChanged || added > 0 || removed > 0)
      {
         LogInfo(std::format("Syncing {} {} to {} {} {} {}",
                             utils::GetServerName(utils::GetFormattedPlex(), plexApi->GetName()),
                             utils::GetTag("collection", playlistName),
                             utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                             utils::GetTag("added", added),
                             utils::GetTag("removed", removed),
                             utils::GetTag("reordered", orderChanged ? "true" : "false")));
      }
   }

   void PlaylistSyncService::SyncEmbyPlaylist(PlexApi* plexApi, EmbyApi* embyApi, const PlexCollection& plexCollection)
   {
      std::vector<std::string> updatedPlaylistIds;
      for (auto& item : plexCollection.items)
      {
         bool foundItem{false};
         for (auto& path : item.paths)
         {
            auto embyId{embyApi->GetItemIdFromPath(path)};
            if (embyId.has_value())
            {
               foundItem = true;
               updatedPlaylistIds.emplace_back(embyId.value());
               break;
            }
         }

         if (!foundItem)
         {
            LogWarning(std::format("{} sync {} {} item not found {}",
                                   utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                   utils::GetServerName(utils::GetFormattedPlex(), plexApi->GetName()),
                                   utils::GetTag("collection", plexCollection.name),
                                   utils::GetTag("item", item.title)));
         }
      }

      if (embyApi->GetPlaylistExists(plexCollection.name))
      {
         UpdateEmbyPlaylist(plexApi, embyApi, plexCollection.name, updatedPlaylistIds);
      }
      else
      {
         embyApi->CreatePlaylist(plexCollection.name, updatedPlaylistIds);

         LogInfo(std::format("Creating {} {} on {}",
                             utils::GetServerName(utils::GetFormattedPlex(), plexApi->GetName()),
                             utils::GetTag("collection", plexCollection.name),
                             utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName())));
      }
   }

   void PlaylistSyncService::SyncPlexCollection(PlexApi* plexApi, EmbyApi* embyApi, const PlaylistPlexCollection& collection)
   {
      auto plexCollection{plexApi->GetCollection(collection.library, collection.collectionName)};
      if (plexCollection.has_value())
      {
         SyncEmbyPlaylist(plexApi, embyApi, plexCollection.value());
      }
   }

   void PlaylistSyncService::Run()
   {
      for (auto& plexCollection : plexCollections_)
      {
         auto* plexApi{GetApiManager()->GetPlexApi(plexCollection.server)};
         if (plexApi->GetValid())
         {
            for (const auto& embyServer : plexCollection.embyServers)
            {
               auto* embyApi{GetApiManager()->GetEmbyApi(embyServer)};
               if (embyApi->GetValid())
               {
                  SyncPlexCollection(plexApi, embyApi, plexCollection);
               }
            }
         }
      }
   }
}