#include "tmdb-emby-playlist-sync.h"

#include "services/service-utils.h"

#include <warp/log/log-utils.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <thread>
#include <unordered_set>
#include <utility>

namespace loomis
{
   namespace
   {
      constexpr auto EMBY_PLAYLIST_UPDATE_WAIT(std::chrono::seconds(1));
      constexpr auto PLAYLIST_UPDATE_WAIT_TIME(std::chrono::milliseconds(100));

      struct PlaylistSyncResult
      {
         std::size_t added{0};
         std::size_t removed{0};
         bool reordered{false};
         bool creationRequired{false};
         bool successful{true};
      };

      struct PlaylistChanges
      {
         std::vector<std::string> addIds;
         std::vector<std::string> deleteIds;
      };

      PlaylistChanges BuildPlaylistChanges(
         const warp::EmbyPlaylist& playlist,
         const std::vector<std::string>& targetIds)
      {
         std::unordered_set<std::string_view> targetIdSet;
         targetIdSet.reserve(targetIds.size());
         for (const auto& id : targetIds)
            targetIdSet.emplace(id);

         std::unordered_set<std::string_view> currentItemIds;
         currentItemIds.reserve(playlist.items.size());
         PlaylistChanges changes;
         changes.deleteIds.reserve(playlist.items.size());
         for (const auto& item : playlist.items)
         {
            const bool firstOccurrence = currentItemIds.emplace(item.id).second;
            if (!targetIdSet.contains(item.id) || !firstOccurrence)
               changes.deleteIds.emplace_back(item.playlistId);
         }

         changes.addIds.reserve(targetIds.size());
         for (const auto& targetId : targetIds)
         {
            if (!currentItemIds.contains(targetId))
               changes.addIds.emplace_back(targetId);
         }

         return changes;
      }

      bool WouldRequireReordering(
         const warp::EmbyPlaylist& playlist,
         const std::vector<std::string>& targetIds)
      {
         std::unordered_set<std::string_view> currentItemIds;
         currentItemIds.reserve(playlist.items.size());
         for (const auto& item : playlist.items)
            currentItemIds.emplace(item.id);

         std::unordered_set<std::string_view> targetIdSet;
         targetIdSet.reserve(targetIds.size());
         for (const auto& id : targetIds)
            targetIdSet.emplace(id);

         std::vector<std::string_view> plannedOrder;
         plannedOrder.reserve(targetIds.size());
         for (const auto& item : playlist.items)
         {
            if (targetIdSet.contains(item.id))
               plannedOrder.emplace_back(item.id);
         }
         for (const auto& id : targetIds)
         {
            if (!currentItemIds.contains(id))
               plannedOrder.emplace_back(id);
         }

         return plannedOrder.size() != targetIds.size() ||
            !std::ranges::equal(plannedOrder, targetIds);
      }

      struct PlaylistUpdateResult
      {
         std::optional<warp::EmbyPlaylist> playlist;
         bool successful{true};
      };

      PlaylistUpdateResult ApplyPlaylistChanges(
         warp::EmbyApi& embyApi,
         warp::EmbyPlaylist playlist,
         const PlaylistChanges& changes,
         std::string_view playlistName,
         std::size_t targetSize,
         ServiceLogger logger)
      {
         bool successful = true;
         if (!changes.addIds.empty() && !embyApi.AddPlaylistItems(playlist.id, changes.addIds))
         {
            logger.LogWarning(
               "{} failed to add {} to {}",
               embyApi.GetPrettyName(),
               warp::GetTag("item_count", changes.addIds.size()),
               warp::GetTag("playlist", playlist.name));
            successful = false;
         }

         if (!changes.deleteIds.empty() && !embyApi.RemovePlaylistItems(playlist.id, changes.deleteIds))
         {
            logger.LogWarning(
               "{} failed to remove {} from {}",
               embyApi.GetPrettyName(),
               warp::GetTag("item_count", changes.deleteIds.size()),
               warp::GetTag("playlist", playlist.name));
            successful = false;
         }

         if (!changes.addIds.empty() || !changes.deleteIds.empty())
         {
            std::this_thread::sleep_for(EMBY_PLAYLIST_UPDATE_WAIT);
            auto updatedPlaylist = embyApi.GetPlaylist(playlistName);
            if (!updatedPlaylist)
            {
               logger.LogWarning(
                  "{} failed to retrieve {} after updating",
                  embyApi.GetPrettyName(),
                  warp::GetTag("playlist", playlistName));
               return {std::nullopt, false};
            }
            playlist = std::move(*updatedPlaylist);
         }

         if (playlist.items.size() != targetSize)
         {
            logger.LogWarning(
               "{} playlist update failed. {} should be {} but is {}",
               embyApi.GetPrettyName(),
               warp::GetTag("playlist", playlist.name),
               warp::GetTag("length", targetSize),
               warp::GetTag("reported_length", playlist.items.size()));
            return {std::nullopt, false};
         }

         return {std::move(playlist), successful};
      }

      struct PlaylistReorderResult
      {
         bool reordered{false};
         bool successful{true};
      };

      PlaylistReorderResult ReorderPlaylist(
         warp::EmbyApi& embyApi,
         const warp::EmbyPlaylist& playlist,
         const std::vector<std::string>& targetIds,
         ServiceLogger logger)
      {
         PlaylistReorderResult result;
         struct MoveTracker
         {
            std::string_view id;
            std::string_view playlistId;
         };
         std::vector<MoveTracker> virtualItems;
         virtualItems.reserve(playlist.items.size());
         for (const auto& item : playlist.items)
            virtualItems.push_back({item.id, item.playlistId});

         for (std::size_t index = 0; index < targetIds.size(); ++index)
         {
            if (virtualItems[index].id == targetIds[index])
               continue;

            auto itemToMove = std::ranges::find_if(
               virtualItems | std::views::drop(index),
               [&](const auto& item) { return item.id == targetIds[index]; });

            if (itemToMove == virtualItems.end())
               continue;

            if (embyApi.MovePlaylistItem(playlist.id, itemToMove->playlistId, static_cast<uint32_t>(index)))
            {
               const auto movedItem = *itemToMove;
               virtualItems.erase(itemToMove);
               virtualItems.insert(virtualItems.begin() + static_cast<std::ptrdiff_t>(index), movedItem);
               result.reordered = true;
               std::this_thread::sleep_for(PLAYLIST_UPDATE_WAIT_TIME);
            }
            else
            {
               logger.LogWarning(
                  "{} failed to reorder {}",
                  embyApi.GetPrettyName(),
                  warp::GetTag("playlist", playlist.name));
               result.successful = false;
            }
         }

         return result;
      }

      PlaylistSyncResult SyncPlaylist(
         warp::EmbyApi& embyApi,
         std::string_view playlistName,
         const std::vector<std::string>& targetIds,
         ServiceLogger logger,
         bool dryRun)
      {
         PlaylistSyncResult result;
         std::vector<std::string> uniqueTargetIds;
         uniqueTargetIds.reserve(targetIds.size());
         std::unordered_set<std::string_view> targetIdSet;
         targetIdSet.reserve(targetIds.size());
         for (const auto& id : targetIds)
         {
            if (targetIdSet.emplace(id).second)
               uniqueTargetIds.emplace_back(id);
         }

         auto currentPlaylist = embyApi.GetPlaylist(playlistName);
         if (!currentPlaylist)
         {
            result.creationRequired = true;
            if (!dryRun)
               embyApi.CreatePlaylist(playlistName, uniqueTargetIds);
            return result;
         }

         const auto changes = BuildPlaylistChanges(*currentPlaylist, uniqueTargetIds);
         result.added = changes.addIds.size();
         result.removed = changes.deleteIds.size();

         if (dryRun)
         {
            result.reordered = WouldRequireReordering(*currentPlaylist, uniqueTargetIds);
            return result;
         }

         auto updateResult = ApplyPlaylistChanges(
            embyApi,
            std::move(*currentPlaylist),
            changes,
            playlistName,
            uniqueTargetIds.size(),
            logger);
         result.successful = updateResult.successful;
         if (!updateResult.playlist)
            return result;

         const auto reorderResult = ReorderPlaylist(embyApi, *updateResult.playlist, uniqueTargetIds, logger);
         result.reordered = reorderResult.reordered;
         result.successful = result.successful && reorderResult.successful;
         return result;
      }
   } // namespace

   TmdbEmbyPlaylistSync::TmdbEmbyPlaylistSync(bool dryRun, ServiceLogger logger)
      : logger_(logger)
      , dryRun_(dryRun)
      , dryRunText_(GetDryRunText(dryRun))
   {
   }

   std::optional<TmdbSyncStats> TmdbEmbyPlaylistSync::Sync(
      warp::EmbyApi& embyApi,
      std::string_view playlistName,
      const std::vector<std::string>& targetIds)
   {
      const auto result = SyncPlaylist(embyApi, playlistName, targetIds, logger_, dryRun_);
      if (result.creationRequired)
      {
         logger_.LogInfo(
            "{}Creating TMDB {} on {}",
            dryRunText_,
            warp::GetTag("playlist", playlistName),
            embyApi.GetPrettyName());
         return std::nullopt;
      }
      else if (result.successful && (result.added > 0 || result.removed > 0 || result.reordered))
      {
         return TmdbSyncStats{
            .added = result.added,
            .removed = result.removed,
            .reordered = result.reordered};
      }

      return std::nullopt;
   }
} // namespace loomis
