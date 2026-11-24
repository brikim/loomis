#pragma once

#include "services/service-logger.h"
#include "services/tmdb-sync/tmdb-client.h"
#include "services/tmdb-sync/tmdb-emby-sync.h"
#include "services/tmdb-sync/tmdb-media-source.h"
#include "services/tmdb-sync/tmdb-plex-sync.h"

#include "config-reader/config-reader-types.h"

#include <memory>
#include <string>
#include <string_view>

namespace loomis
{
   class TmdbPlaylist
   {
   public:
      [[nodiscard]] static std::unique_ptr<TmdbPlaylist> Create(
         const TmdbPlaylistConfig& config,
         std::shared_ptr<TmdbClient> tmdbClient,
         std::shared_ptr<warp::ApiManager> apiManager,
         bool dryRun,
         ServiceLogger logger);

      TmdbPlaylist(
         TmdbPlaylistConfig config,
         std::shared_ptr<TmdbClient> tmdbClient,
         std::unique_ptr<TmdbMediaSource> mediaSource,
         std::shared_ptr<warp::ApiManager> apiManager,
         bool dryRun,
         ServiceLogger logger);
      ~TmdbPlaylist() = default;

      void Sync();

   private:
      TmdbPlaylistConfig config_;
      std::shared_ptr<TmdbClient> tmdbClient_;
      std::unique_ptr<TmdbMediaSource> mediaSource_;
      TmdbEmbySync embySync_;
      TmdbPlexSync plexSync_;
      ServiceLogger logger_;
      std::string_view dryRunText_;
   };
} // namespace loomis
