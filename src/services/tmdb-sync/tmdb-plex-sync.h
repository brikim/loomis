#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"
#include "services/tmdb-sync/tmdb-plex-collection-sync.h"
#include "services/tmdb-sync/tmdb-sync-result.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace warp
{
   class ApiManager;
}

namespace loomis
{
   class TmdbPlexSync
   {
   public:
      TmdbPlexSync(std::shared_ptr<warp::ApiManager> apiManager,
                   bool dryRun,
                   ServiceLogger logger);

      std::vector<TmdbSyncLogEntry> Sync(const TmdbPlaylistConfig& config, const std::vector<int64_t>& tmdbIds);

   private:
      std::shared_ptr<warp::ApiManager> apiManager_;
      TmdbPlexCollectionSync collectionSync_;
      ServiceLogger logger_;
      std::string_view dryRunText_;
   };
} // namespace loomis
