#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"
#include "services/tmdb-sync/tmdb-emby-playlist-sync.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace warp
{
   class ApiManager;
}

namespace loomis
{
   class TmdbEmbySync
   {
   public:
      TmdbEmbySync(std::shared_ptr<warp::ApiManager> apiManager,
                   bool dryRun,
                   ServiceLogger logger);

      void Sync(const TmdbPlaylistConfig& config, const std::vector<int64_t>& tmdbIds);

   private:
      std::shared_ptr<warp::ApiManager> apiManager_;
      TmdbEmbyPlaylistSync playlistSync_;
      ServiceLogger logger_;
      std::string dryRunText_;
   };
} // namespace loomis
