#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-manager.h>
#include <warp/api/api-tracearr-types.h>
#include <warp/types.h>

#include <filesystem>
#include <string>
#include <string_view>

namespace loomis
{
   class StateSyncEmbyUser
   {
   public:
      StateSyncEmbyUser(bool dryRun,
                        ServiceLogger logger);
      ~StateSyncEmbyUser() = default;

      struct EmbySyncState
      {
         const warp::TracearrHistoryItem& item;
         const std::filesystem::path& mediaPath;
         warp::EmbyApi* embyApi{nullptr};
         std::string_view embyUserId;
      };
      void SyncState(EmbySyncState syncState, std::string& syncResults);

   private:
      bool SyncWatchedState(EmbySyncState& syncState, std::string_view id);
      bool SyncPlayState(EmbySyncState& syncState, std::string_view id);

      ServiceLogger logger_;
      bool dryRun_{false};
   };
}