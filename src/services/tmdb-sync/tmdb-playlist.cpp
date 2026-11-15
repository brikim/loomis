#include "tmdb-playlist.h"

#include "services/tmdb-sync/tmdb-media-source-factory.h"
#include "services/tmdb-sync/tmdb-playlist-type.h"
#include "services/tmdb-sync/tmdb-string-utils.h"

#include <warp/log/log-utils.h>

namespace loomis
{
   TmdbPlaylist::TmdbPlaylist(const TmdbPlaylistConfig& config,
                              std::shared_ptr<TmdbClient> tmdbClient,
                              bool dryRun,
                              ServiceLogger logger)
      : config_(config)
      , tmdbClient_(std::move(tmdbClient))
      , logger_(logger)
      , dryRunText_(dryRun ? "[DRY RUN] " : "")
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

      const auto mediaType = ParseTmdbMediaType(config_.mediaType);
      if (!mediaType)
      {
         logger_.LogError(
            "{}{} has invalid {}. Supported values are: {}.",
            dryRunText_,
            warp::GetTag("playlist", config_.name),
            warp::GetTag("media type", config_.mediaType),
            JoinCommaSeparated(TMDB_MEDIA_TYPE_NAMES));
         return;
      }

      if (config_.embyServers.empty() && config_.plexServers.empty())
      {
         logger_.LogError("{}No servers configured. At least one Plex or Emby server is required.",
                          dryRunText_);
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

      mediaSource_ = CreateTmdbMediaSource(config_, *playlistType, *mediaType, logger_, dryRunText_);
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

      logger_.LogInfo(
         "{}Starting TMDB playlist sync for {} ({})",
         dryRunText_,
         config_.name,
         config_.type);

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

      logger_.LogInfo(
         "{}TMDB {} returned {} media items for {}",
         dryRunText_,
         config_.type,
         mediaItems->size(),
         config_.name);

      for (const auto& item : *mediaItems)
      {
         const auto mediaType = GetTmdbMediaTypeString(item.mediaType);
         logger_.LogInfo(
            "{}TMDB {} id from {}: {}",
            dryRunText_,
            mediaType,
            config_.name,
            item.id);
      }
   }
} // namespace loomis
