#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"
#include "services/state-sync-tracearr/emby-user.h"
#include "services/state-sync-tracearr/plex-user.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-tracearr-types.h>
#include <warp/types.h>

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace loomis
{
   class StateSyncUser
   {
   public:
      StateSyncUser(const UserSyncTracearrConfig& config,
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

      // Returns no duplicates. These will be thrown out and the latest item of the duplicates will be returned
      std::vector<const warp::TautulliHistoryItem*> GetConsolidatedPlexHistory(const warp::TautulliHistoryItems& historyItems);
      std::vector<const warp::JellystatHistoryItem*> GetConsolidatedEmbyHistory(const warp::JellystatHistoryItems& historyItems);

      bool valid_{false};
      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger logger_;
      std::string dryRunText_;

      std::string tracearrUserName_;
      std::vector<std::unique_ptr<StateSyncPlexUser>> plexUsers_;
      std::vector<std::unique_ptr<StateSyncEmbyUser>> embyUsers_;
   };
}