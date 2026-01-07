#pragma once

#include "api/api-manager.h"
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
                     ApiManager* apiManager,
                     WatchStateLogger logger);
      virtual ~WatchStateUser() = default;

      [[nodiscard]] bool GetValid() const;

      void Sync();

   private:
      void UpdateAllUsers();

      void SyncPlexWatchState(const TautulliHistoryItem* item);
      void SyncPlexPlayState(const TautulliHistoryItem* item);
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
      std::vector<const TautulliHistoryItem*> GetConsolidatedHistory(const TautulliHistoryItems& historyItems);

      std::unordered_map<int32_t, std::string> GetPlexPathsForHistoryItems(std::string_view server, const std::vector<const TautulliHistoryItem*> historyItems);

      bool valid_{false};
      ApiManager* apiManager_{nullptr};
      WatchStateLogger logger_;

      std::vector<std::unique_ptr<PlexUser>> plexUsers_;
      std::vector<std::unique_ptr<EmbyUser>> embyUsers_;
   };
}