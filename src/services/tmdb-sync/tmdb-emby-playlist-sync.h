#pragma once

#include "services/service-logger.h"

#include <warp/api/api-emby.h>

#include <string>
#include <string_view>
#include <vector>

namespace loomis
{
   class TmdbEmbyPlaylistSync
   {
   public:
      TmdbEmbyPlaylistSync(bool dryRun, ServiceLogger logger);

      void Sync(
         warp::EmbyApi& embyApi,
         std::string_view playlistName,
         const std::vector<std::string>& targetIds);

   private:
      ServiceLogger logger_;
      bool dryRun_{false};
      std::string_view dryRunText_;
   };
} // namespace loomis
