#include "tmdb-playlist.h"

#include "services/service-utils.h"
#include "services/tmdb-sync/tmdb-media-source-factory.h"
#include "services/tmdb-sync/tmdb-playlist-type.h"

#include <warp/log/log-utils.h>
#include <warp/utils.h>

#include <algorithm>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

namespace loomis
{
   namespace
   {
      struct SyncLogGroup
      {
         TmdbSyncStats stats;
         std::string destinations;
      };

      void LogSyncResults(
         const std::vector<TmdbSyncLogEntry>& syncLogEntries,
         ServiceLogger logger,
         std::string_view dryRunText,
         std::string_view playlistName)
      {
         std::vector<SyncLogGroup> syncLogGroups;
         for (const auto& entry : syncLogEntries)
         {
            const auto group = std::ranges::find_if(syncLogGroups, [&](const auto& candidate) {
               return candidate.stats == entry.stats;
            });
            if (group == syncLogGroups.end())
            {
               syncLogGroups.push_back(
                  SyncLogGroup{
                     .stats = entry.stats,
                     .destinations = entry.destination});
            }
            else
            {
               group->destinations = warp::BuildSyncServerString(
                  group->destinations,
                  entry.destination,
                  "");
            }
         }

         for (const auto& group : syncLogGroups)
         {
            logger.LogInfo(
               "{}Syncing TMDB {} to {} {} {} {}",
               dryRunText,
               warp::GetTag("playlist", playlistName),
               group.destinations,
               warp::GetTag("added", group.stats.added),
               warp::GetTag("removed", group.stats.removed),
               warp::GetTag("reordered", group.stats.reordered ? "true" : "false"));
         }
      }
   } // namespace

   std::unique_ptr<TmdbPlaylist> TmdbPlaylist::Create(
      const TmdbPlaylistConfig& config,
      std::shared_ptr<TmdbClient> tmdbClient,
      std::shared_ptr<warp::ApiManager> apiManager,
      bool dryRun,
      ServiceLogger logger)
   {
      const auto dryRunText = GetDryRunText(dryRun);
      const auto playlistType = ParseTmdbPlaylistType(config.type);
      if (!playlistType)
      {
         logger.LogError(
            "{}Unsupported TMDB {}",
            dryRunText,
            warp::GetTag("playlist type", config.type));
         return nullptr;
      }

      if (config.embyServers.empty() && config.plexServers.empty())
      {
         logger.LogError("{}At least one Plex or Emby server is required for TMDB playlist sync.", dryRunText);
         return nullptr;
      }

      if (!tmdbClient)
      {
         logger.LogError("{}TMDB client is unavailable.", dryRunText);
         return nullptr;
      }

      if (!config.maxItems)
      {
         logger.LogError(
            "{}Playlist {} requires {} to be set.",
            dryRunText,
            warp::GetTag("playlist", config.name),
            warp::GetTag("config", "max_items"));
         return nullptr;
      }

      auto mediaSource = CreateTmdbMediaSource(config, *playlistType, logger, dryRunText);
      if (!mediaSource)
         return nullptr;

      return std::make_unique<TmdbPlaylist>(
         config,
         std::move(tmdbClient),
         std::move(mediaSource),
         std::move(apiManager),
         dryRun,
         logger);
   }

   TmdbPlaylist::TmdbPlaylist(
      TmdbPlaylistConfig config,
      std::shared_ptr<TmdbClient> tmdbClient,
      std::unique_ptr<TmdbMediaSource> mediaSource,
      std::shared_ptr<warp::ApiManager> apiManager,
      bool dryRun,
      ServiceLogger logger)
      : config_(std::move(config))
      , tmdbClient_(std::move(tmdbClient))
      , mediaSource_(std::move(mediaSource))
      , embySync_(apiManager, dryRun, logger)
      , plexSync_(std::move(apiManager), dryRun, logger)
      , logger_(logger)
      , dryRunText_(GetDryRunText(dryRun))
   {
   }

   void TmdbPlaylist::Sync()
   {
      const auto mediaItems = mediaSource_->Fetch(*tmdbClient_, *config_.maxItems);
      if (!mediaItems)
      {
         logger_.LogError(
            "{}Failed to fetch TMDB media items for playlist {}",
            dryRunText_,
            config_.name);
         return;
      }

      if (mediaItems->empty())
      {
         logger_.LogWarning(
            "{}TMDB {} returned no media items for {}",
            dryRunText_,
            config_.type,
            config_.name);
         return;
      }

      auto syncLogEntries = embySync_.Sync(config_, *mediaItems);
      auto plexSyncLogEntries = plexSync_.Sync(config_, *mediaItems);
      syncLogEntries.insert(
         syncLogEntries.end(),
         std::make_move_iterator(plexSyncLogEntries.begin()),
         std::make_move_iterator(plexSyncLogEntries.end()));
      LogSyncResults(syncLogEntries, logger_, dryRunText_, config_.name);
   }
} // namespace loomis
