#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"
#include "services/state-sync/emby-user.h"
#include "services/state-sync/plex-user.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-tracearr-types.h>
#include <warp/types.h>

#include <memory>
#include <vector>

namespace loomis
{
   class StateSyncUser
   {
   public:
      StateSyncUser(bool dryRun,
                    std::shared_ptr<warp::ApiManager> apiManager,
                    ServiceLogger logger);
      ~StateSyncUser() = default;

      void Sync(const warp::TracearrHistoryItem& historyItem);

   private:
      void SyncPlexState(const warp::TracearrUser& user, const warp::TracearrHistoryItem& historyItem);
      void SyncEmbyState(const warp::TracearrUser& user, const warp::TracearrHistoryItem& historyItem);

      struct LogSyncData
      {
         std::string_view server;
         std::string_view user;
         std::string_view name;
         bool watched{false};
         int32_t playbackPercentage{0};
         std::string_view syncResults;
      };
      void LogSyncSummary(const LogSyncData& syncSummary);

      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger logger_;
      std::string dryRunText_;
      warp::TracearrApi* tracearrApi_{nullptr};

      std::unique_ptr<StateSyncPlexUser> plexUser_;
      std::unique_ptr<StateSyncEmbyUser> embyUser_;
   };
}