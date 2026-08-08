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
      StateSyncUser(const UserSyncConfig& config,
                    bool dryRun,
                    std::shared_ptr<warp::ApiManager> apiManager,
                    ServiceLogger logger);
      ~StateSyncUser() = default;

      [[nodiscard]] bool GetValid() const;
      [[nodiscard]] std::string GetServerAndUserName() const;

      void Sync(const std::vector<const warp::TracearrHistoryItem*>& historyItems);

   private:
      void UpdateAllUsers();

      void SyncPlexState(const warp::TracearrHistoryItem* historyItem, StateSyncPlexUser& plexUser);
      void SyncEmbyState(const warp::TracearrHistoryItem* historyItem, StateSyncEmbyUser& embyUser);

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

      bool valid_{false};
      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger logger_;
      std::string dryRunText_;

      std::string tracearrUserName_;
      std::vector<std::unique_ptr<StateSyncPlexUser>> plexUsers_;
      std::vector<std::unique_ptr<StateSyncEmbyUser>> embyUsers_;
   };
}