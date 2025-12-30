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

   std::pair<size_t, size_t> PlaylistSyncService::AddRemoveEmbyPlaylistItems(EmbyApi* embyApi, const EmbyPlaylist& currentPlaylist, const std::vector<std::string>& updatedPlaylistIds)
   {
      std::vector<std::string> addIds;
      for (const auto& updatedId : updatedPlaylistIds)
      {
         auto found = std::ranges::any_of(currentPlaylist.items, [&updatedId](const auto& item) {
            return item.id == updatedId;
         });

         if (!found)
         {
            addIds.emplace_back(updatedId);
         }
      }

      std::vector<std::string> deleteIds;
      for (const auto& item : currentPlaylist.items)
      {
         auto notFound = std::ranges::none_of(updatedPlaylistIds, [&item](const auto& id) {
            return item.id == id;
         });

         if (notFound)
         {
            deleteIds.emplace_back(item.id);
         }
      }

      if (!addIds.empty() && !embyApi->AddPlaylistItems(currentPlaylist.id, addIds))
      {
         LogWarning(std::format("{} failed to add {} to {}",
                                utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                utils::GetTag("item_count", std::to_string(addIds.size())),
                                utils::GetTag("playlist", currentPlaylist.name)));
      }

      if (!deleteIds.empty() && !embyApi->RemovePlaylistItems(currentPlaylist.id, deleteIds))
      {
         LogWarning(std::format("{} failed to remove {} from {}",
                                utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                utils::GetTag("item_count", std::to_string(addIds.size())),
                                utils::GetTag("playlist", currentPlaylist.name)));
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

   void PlaylistSyncService::UpdateEmbyPlaylist(PlexApi* plexApi, EmbyApi* embyApi, std::string_view playlistName, const std::vector<std::string>& correctIds)
   {
      auto currentEmbyPlaylist{embyApi->GetPlaylist(playlistName)};
      if (!currentEmbyPlaylist.has_value())
      {
         LogWarning(std::format("{} failed to retrieve {} on update",
                                utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                utils::GetTag("playlist", playlistName)));
         return;
      }

      auto addRemoveResult{AddRemoveEmbyPlaylistItems(embyApi, currentEmbyPlaylist.value(), correctIds)};
      if (addRemoveResult.first > 0 || addRemoveResult.second > 0)
      {
         // Sleep to give Emby time to update for the next step
         std::this_thread::sleep_for(std::chrono::seconds(timeForEmbyUpdateSec_));

         // Update the playlist to get the latest since it was changed
         currentEmbyPlaylist = embyApi->GetPlaylist(playlistName);
      }

      // Detect an error condition. Will not proceed if detected.
      if (!currentEmbyPlaylist.has_value() || currentEmbyPlaylist.value().items.size() != correctIds.size())
      {
         if (!currentEmbyPlaylist.has_value())
         {
            LogWarning(std::format("{} failed to retrieve {} on update",
                                   utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                   utils::GetTag("playlist", playlistName)));
         }
         else
         {
            LogWarning(std::format("{} sync {} {} playlist updated failed. Playlist length should be {} but {}",
                                   utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                                   utils::GetServerName(utils::GetFormattedPlex(), plexApi->GetName()),
                                   utils::GetTag("collection", playlistName),
                                   utils::GetTag("length", std::to_string(correctIds.size())),
                                   utils::GetTag("reported_length", std::to_string(currentEmbyPlaylist.value().items.size()))));
         }
         return;
      }

      // If we are here the size of the updated playlist and the updated playlist ids is the same
      auto orderChanged = !std::ranges::equal(correctIds, currentEmbyPlaylist.value().items, std::equal_to<>{}, {}, & EmbyPlaylistItem::id);
      if (orderChanged)
      {
         for (int32_t i = 0; i < correctIds.size(); ++i)
         {
            // If the update was successful get the new playlist. This is needed to prevent
            // unnecessary moves to the emby server.
            if (UpdateToCorrectLocation(embyApi, currentEmbyPlaylist.value(), correctIds[i], i))
            {
               // Moved an item. Get the new playlist
               currentEmbyPlaylist = embyApi->GetPlaylist(playlistName);
            }
         }
      }

      // Log results if needed
      if (orderChanged || addRemoveResult.first > 0 || addRemoveResult.second > 0)
      {
         LogInfo(std::format("Syncing {} {} to {} {} {} {}",
                             utils::GetServerName(utils::GetFormattedPlex(), plexApi->GetName()),
                             utils::GetTag("collection", playlistName),
                             utils::GetServerName(utils::GetFormattedEmby(), embyApi->GetName()),
                             utils::GetTag("added", std::to_string(addRemoveResult.first)),
                             utils::GetTag("deleted", std::to_string(addRemoveResult.second)),
                             utils::GetTag("reordered", orderChanged ? "True" : "False")));
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
      const auto& plexCollection{plexApi->GetCollection(collection.library, collection.collectionName)};
      if (plexCollection.valid)
      {
         SyncEmbyPlaylist(plexApi, embyApi, plexCollection);
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