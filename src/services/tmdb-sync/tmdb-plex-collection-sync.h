#pragma once

#include "services/service-logger.h"
#include "services/tmdb-sync/tmdb-sync-result.h"

#include <warp/api/api-plex.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace loomis
{
   class TmdbPlexCollectionSync
   {
   public:
      TmdbPlexCollectionSync(bool dryRun, ServiceLogger logger);

      std::optional<TmdbSyncStats> Sync(
         warp::PlexApi& plexApi,
         std::string_view libraryName,
         std::string_view collectionName,
         std::span<const std::string> targetIds);

      std::optional<TmdbSyncStats> Sync(
         warp::PlexApi& plexApi,
         std::string_view libraryName,
         std::string_view collectionName,
         std::span<const std::string_view> targetIds);

   private:
      ServiceLogger logger_;
      bool dryRun_{false};
      std::string_view dryRunText_;
   };
} // namespace loomis
