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
               const std::shared_ptr<ApiManager>& apiManager,
               WatchStateLogger logger);
      virtual ~PlexUser() = default;

      [[nodiscard]] bool GetValid() const;
      [[nodiscard]] std::string GetServerAndUserName() const;
      [[nodiscard]] int32_t GetId() const;
      [[nodiscard]] std::string_view GetServerName() const;
      [[nodiscard]] std::string_view GetTypeAndServerName() const;
      [[nodiscard]] std::string_view GetUser() const;
      [[nodiscard]] std::optional<TautulliHistoryItems> GetWatchHistory(std::string_view historyDate);

      void Update();

      void SyncStateWithPlex();

      struct EmbySyncState
      {
         const std::string& name;
         const std::string& mediaPath;
         const std::string& path;
         bool watched{false};
         int32_t playbackPercentage{0};
         const std::string& timeWatched;
      };
      void SyncStateWithEmby(const EmbySyncState& syncState, std::string& syncResults);

   private:
      bool SyncEmbyWatchedState(const EmbySyncState& syncState);
      bool SyncEmbyPlayState(const EmbySyncState& syncState);

      bool valid_{false};
      WatchStateLogger logger_;
      ServerUser config_;
      std::string typeServerName_;

      PlexApi* api_{nullptr};
      TautulliApi* trackerApi_{nullptr};

      TautulliUserInfo userInfo_;
   };
}