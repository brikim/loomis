#pragma once

#include "api/api-manager.h"
#include "config-reader/config-reader-types.h"
#include "services/watch-state-sync/emby-user.h"
#include "services/watch-state-sync/plex-user.h"
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
                     const std::function<void(LogType, const std::string&)>& logFunc);
      virtual ~WatchStateUser() = default;

      [[nodiscard]] bool GetValid() const;

      void Sync();

   private:
      void UpdateAllUsers();

      void SyncPlexWatchState(const TautulliHistoryItem* item);
      void SyncPlexPlayState(const TautulliHistoryItem* item);
      void SyncPlexState(PlexUser* plexUser, std::string_view historyDate);

      // Returns no duplicates. These will be thrown out and the latest item of the duplicates will be returned
      std::vector<const TautulliHistoryItem*> GetConsolodatedHistory(const TautulliHistoryItems& historyItems);

      std::unordered_map<int32_t, std::string> GetPlexPathsForHistoryItems(std::string_view server, const std::vector<const TautulliHistoryItem*> historyItems);

      bool valid_{false};
      ApiManager* apiManager_{nullptr};
      std::function<void(LogType, const std::string&)> logFunc_;

      std::vector<std::unique_ptr<PlexUser>> plexUsers_;
      std::vector<std::unique_ptr<EmbyUser>> embyUsers_;
   };
}