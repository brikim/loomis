#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"
#include "services/watch-state-sync/emby-user.h"
#include "services/watch-state-sync/plex-user.h"

#include <warp/api/api-jellystat-types.h>
#include <warp/api/api-manager.h>
#include <warp/api/api-tautulli-types.h>
#include <warp/types.h>

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace loomis
{
   class WatchStateUser
   {
   public:
      WatchStateUser(const UserSyncConfig& config,
                     std::shared_ptr<warp::ApiManager> apiManager,
                     ServiceLogger logger);
      ~WatchStateUser() = default;

      [[nodiscard]] bool GetValid() const;
      [[nodiscard]] std::string GetServerAndUserName() const;

      void Sync();

   private:
      void UpdateAllUsers();

      void SyncPlexState(PlexUser& plexUser, std::string_view historyDate, int64_t epochHistoryTime);
      void SyncEmbyState(EmbyUser& embyUser);

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

      std::unordered_map<std::string, std::filesystem::path> GetPlexPathsForHistoryItems(std::string_view server,
                                                                                         const std::vector<const warp::TautulliHistoryItem*> historyItems);

      bool valid_{false};
      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger logger_;

      std::vector<std::unique_ptr<PlexUser>> plexUsers_;
      std::vector<std::unique_ptr<EmbyUser>> embyUsers_;
   };
}