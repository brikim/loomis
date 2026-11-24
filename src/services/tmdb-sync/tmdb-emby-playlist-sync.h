#pragma once

#include "services/service-logger.h"
#include "services/tmdb-sync/tmdb-sync-result.h"

#include <warp/api/api-emby.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace loomis
{
   class TmdbEmbyPlaylistSync
   {
   public:
      TmdbEmbyPlaylistSync(bool dryRun, ServiceLogger logger);

      std::optional<TmdbSyncStats> Sync(
         warp::EmbyApi& embyApi,
         std::string_view playlistName,
         std::span<const std::string> targetIds);

      std::optional<TmdbSyncStats> Sync(
         warp::EmbyApi& embyApi,
         std::string_view playlistName,
         std::span<const std::string_view> targetIds);

   private:
      ServiceLogger logger_;
      bool dryRun_{false};
      std::string_view dryRunText_;
   };
} // namespace loomis
