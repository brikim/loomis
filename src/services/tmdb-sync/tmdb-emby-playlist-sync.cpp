#include "tmdb-emby-playlist-sync.h"

#include "services/service-utils.h"
#include "services/tmdb-sync/tmdb-container-reconciler.h"

#include <warp/log/log-utils.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace loomis
{
   namespace
   {
      constexpr auto EMBY_PLAYLIST_UPDATE_WAIT(std::chrono::seconds(1));
      constexpr auto PLAYLIST_UPDATE_WAIT_TIME(std::chrono::milliseconds(100));

      struct PlaylistUpdateResult
      {
         std::optional<warp::EmbyPlaylist> playlist;
         bool successful{true};
      };

      PlaylistUpdateResult ApplyPlaylistChanges(
         warp::EmbyApi& embyApi,
         warp::EmbyPlaylist& playlist,
         const ContainerChanges& changes,
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

         if (!changes.removeIds.empty() && !embyApi.RemovePlaylistItems(playlist.id, changes.removeIds))
         {
            logger.LogWarning(
               "{} failed to remove {} from {}",
               embyApi.GetPrettyName(),
               warp::GetTag("item_count", changes.removeIds.size()),
               warp::GetTag("playlist", playlist.name));
            successful = false;
         }

         if (!changes.addIds.empty() || !changes.removeIds.empty())
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

         if (!VerifyContainerSize(
                playlist.items.size(),
                targetSize,
                embyApi.GetPrettyName(),
                "playlist",
                playlist.name,
                logger))
         {
            return {std::nullopt, false};
         }

         return {std::move(playlist), successful};
      }

      ContainerReorderResult ReorderPlaylist(
         warp::EmbyApi& embyApi,
         const warp::EmbyPlaylist& playlist,
         std::span<const std::string_view> targetIds,
         ServiceLogger logger)
      {
         std::vector<ReorderItemTracker> virtualItems;
         virtualItems.reserve(playlist.items.size());
         for (const auto& item : playlist.items)
            virtualItems.push_back({.id = item.id, .moveKey = item.playlistId});

         return ReorderContainer(
            std::move(virtualItems),
            targetIds,
            [&](const ReorderItemTracker& item, std::size_t index, std::optional<std::string_view> /*prevId*/) {
               if (embyApi.MovePlaylistItem(playlist.id, item.moveKey, static_cast<uint32_t>(index)))
               {
                  std::this_thread::sleep_for(PLAYLIST_UPDATE_WAIT_TIME);
                  return true;
               }
               return false;
            },
            embyApi.GetPrettyName(),
            "playlist",
            playlist.name,
            logger);
      }

      ContainerSyncResult SyncPlaylist(
         warp::EmbyApi& embyApi,
         std::string_view playlistName,
         std::span<const std::string_view> targetIds,
         ServiceLogger logger,
         bool dryRun)
      {
         ContainerSyncResult result;

         auto currentPlaylist = embyApi.GetPlaylist(playlistName);
         if (!currentPlaylist)
         {
            result.creationRequired = true;
            if (!dryRun)
            {
               embyApi.CreatePlaylist(playlistName, targetIds);
            }
            return result;
         }

         const auto changes = BuildContainerChanges(
            currentPlaylist->items,
            targetIds,
            // Explicit return types keep these as views; inferred auto could copy the strings.
            [](const warp::EmbyPlaylistItem& item) -> std::string_view { return item.id; },
            [](const warp::EmbyPlaylistItem& item) -> std::string_view { return item.playlistId; });

         result.added = changes.addIds.size();
         result.removed = changes.removeIds.size();

         if (dryRun)
         {
            result.reordered = WouldRequireReordering(
               currentPlaylist->items,
               targetIds,
               // Explicit return types keep these as views; inferred auto could copy the strings.
               [](const warp::EmbyPlaylistItem& item) -> std::string_view { return item.id; });
            return result;
         }

         auto updateResult = ApplyPlaylistChanges(
            embyApi,
            *currentPlaylist,
            changes,
            playlistName,
            targetIds.size(),
            logger);
         result.successful = updateResult.successful;
         if (!updateResult.playlist)
            return result;

         const auto reorderResult = ReorderPlaylist(embyApi, *updateResult.playlist, targetIds, logger);
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
      std::span<const std::string> targetIds)
   {
      const auto uniqueTargetIds = DeduplicateTargetIds(targetIds);
      const auto result = SyncPlaylist(embyApi, playlistName, uniqueTargetIds, logger_, dryRun_);

      return ToSyncStats(
         result,
         "playlist",
         playlistName,
         embyApi.GetPrettyName(),
         dryRunText_,
         dryRun_,
         logger_);
   }

   std::optional<TmdbSyncStats> TmdbEmbyPlaylistSync::Sync(
      warp::EmbyApi& embyApi,
      std::string_view playlistName,
      std::span<const std::string_view> targetIds)
   {
      const auto uniqueTargetIds = DeduplicateTargetIds(targetIds);
      const auto result = SyncPlaylist(embyApi, playlistName, uniqueTargetIds, logger_, dryRun_);

      return ToSyncStats(
         result,
         "playlist",
         playlistName,
         embyApi.GetPrettyName(),
         dryRunText_,
         dryRun_,
         logger_);
   }
} // namespace loomis
