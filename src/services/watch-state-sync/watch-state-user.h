#pragma once

#include "api/api-jellystat-types.h"
#include "api/api-manager.h"
#include "api/api-tautulli-types.h"
#include "config-reader/config-reader-types.h"
#include "services/watch-state-sync/emby-user.h"
#include "services/watch-state-sync/plex-user.h"
#include "services/watch-state-sync/watch-state-logger.h"
#include "types.h"

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
                     std::shared_ptr<ApiManager> apiManager,
                     WatchStateLogger logger);
      virtual ~WatchStateUser() = default;

      [[nodiscard]] bool GetValid() const;
      [[nodiscard]] std::string GetServerAndUserName() const;

      void Sync();

   private:
      void UpdateAllUsers();

      void SyncPlexState(PlexUser& plexUser, std::string_view historyDate);
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
      std::vector<const TautulliHistoryItem*> GetConsolidatedPlexHistory(const TautulliHistoryItems& historyItems);
      std::vector<const JellystatHistoryItem*> GetConsolidatedEmbyHistory(const JellystatHistoryItems& historyItems);

      std::unordered_map<int32_t, std::string> GetPlexPathsForHistoryItems(std::string_view server, const std::vector<const TautulliHistoryItem*> historyItems);

      bool valid_{false};
      std::shared_ptr<ApiManager> apiManager_;
      WatchStateLogger logger_;

      std::vector<std::unique_ptr<PlexUser>> plexUsers_;
      std::vector<std::unique_ptr<EmbyUser>> embyUsers_;
   };
}