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
               return candidate.stats.added == entry.stats.added &&
                  candidate.stats.removed == entry.stats.removed &&
                  candidate.stats.reordered == entry.stats.reordered;
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

   TmdbPlaylist::TmdbPlaylist(const TmdbPlaylistConfig& config,
                              std::shared_ptr<TmdbClient> tmdbClient,
                              std::shared_ptr<warp::ApiManager> apiManager,
                              bool dryRun,
                              ServiceLogger logger)
      : config_(config)
      , tmdbClient_(std::move(tmdbClient))
      , embySync_(apiManager, dryRun, logger)
      , plexSync_(std::move(apiManager), dryRun, logger)
      , logger_(logger)
      , dryRunText_(GetDryRunText(dryRun))
   {
      ErrorCheckConfig();
   }

   void TmdbPlaylist::ErrorCheckConfig()
   {
      const auto playlistType = ParseTmdbPlaylistType(config_.type);
      if (!playlistType)
      {
         logger_.LogError(
            "{}Unsupported TMDB {}",
            dryRunText_,
            warp::GetTag("playlist type", config_.type));
         return;
      }

      if (config_.embyServers.empty() && config_.plexServers.empty())
      {
         logger_.LogError("{}At least one Plex or Emby server is required for TMDB playlist sync.", dryRunText_);
         return;
      }

      if (!tmdbClient_)
      {
         logger_.LogError("{}TMDB client is unavailable.", dryRunText_);
         return;
      }

      if (!config_.maxItems)
      {
         logger_.LogError(
            "{}Playlist {} requires {} to be set.",
            dryRunText_,
            warp::GetTag("playlist", config_.name),
            warp::GetTag("config", "max_items"));
         return;
      }

      mediaSource_ = CreateTmdbMediaSource(config_, *playlistType, logger_, dryRunText_);
      valid_ = mediaSource_ != nullptr;
   }

   bool TmdbPlaylist::IsValid() const
   {
      return valid_;
   }

   void TmdbPlaylist::Sync()
   {
      if (!valid_)
      {
         logger_.LogError(
            "{}Cannot sync invalid TMDB playlist {}.",
            dryRunText_,
            warp::GetTag("playlist", config_.name));
         return;
      }

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
