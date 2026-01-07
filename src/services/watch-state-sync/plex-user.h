#pragma once

#include "api/api-manager.h"
#include "api/api-plex.h"
#include "api/api-tautulli.h"
#include "config-reader/config-reader-types.h"
#include "services/watch-state-sync/watch-state-logger.h"
#include "types.h"

#include <functional>
#include <string>
#include <vector>

namespace loomis
{
   class PlexUser
   {
   public:
      PlexUser(const ServerUser& config,
               ApiManager* apiManager,
               WatchStateLogger logger);
      virtual ~PlexUser() = default;

      [[nodiscard]] bool GetValid() const;
      [[nodiscard]] int32_t GetId() const;
      [[nodiscard]] std::string_view GetServer() const;
      [[nodiscard]] std::string_view GetUser() const;
      [[nodiscard]] std::optional<TautulliHistoryItems> GetWatchHistory(std::string_view historyDate);
      [[nodiscard]] std::string_view GetServerName() const;

      void Update();

      void SyncStateWithPlex(const TautulliHistoryItem* item, std::string_view path, std::string& target);

   private:
      bool valid_{false};
      WatchStateLogger logger_;
      ServerUser config_;
      std::string serverName_;

      PlexApi* api_{nullptr};
      TautulliApi* trackerApi_{nullptr};

      TautulliUserInfo userInfo_;
   };
}