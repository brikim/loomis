#pragma once

#include "api/api-emby.h"
#include "api/api-jellystat.h"
#include "api/api-manager.h"
#include "api/api-tautulli-types.h"
#include "config-reader/config-reader-types.h"
#include "services/watch-state-sync/watch-state-logger.h"
#include "types.h"

#include <functional>

namespace loomis
{
   class EmbyUser
   {
   public:
      EmbyUser(const ServerUser& config,
               ApiManager* apiManager,
               WatchStateLogger logger);
      virtual ~EmbyUser() = default;

      [[nodiscard]] bool GetValid() const;
      [[nodiscard]] std::string_view GetServerName() const;
      [[nodiscard]] std::optional<JellystatHistoryItems> GetWatchHistory();

      void Update();

      struct PlexSyncState
      {
         const std::string& path;
         bool watched{false};
         int32_t playbackPercentage{0};
         int64_t timeWatchedEpoch{0};
      };
      void SyncStateWithPlex(const PlexSyncState& syncState, std::string& syncResults);

   private:
      bool SyncWatchedState(const std::string& plexPath);
      bool SyncPlayState(const PlexSyncState& syncState);

      bool valid_{false};
      WatchStateLogger logger_;
      ServerUser config_;
      std::string userId_;
      std::string serverName_;

      EmbyApi* embyApi_{nullptr};
      JellystatApi* jellystatApi_{nullptr};
   };
}