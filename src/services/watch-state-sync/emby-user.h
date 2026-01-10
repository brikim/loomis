#pragma once

#include "api/api-emby.h"
#include "api/api-jellystat.h"
#include "api/api-manager.h"
#include "api/api-tautulli-types.h"
#include "config-reader/config-reader-types.h"
#include "services/watch-state-sync/watch-state-logger.h"
#include "types.h"

#include <chrono>
#include <functional>

namespace loomis
{
   class EmbyUser
   {
   public:
      EmbyUser(const ServerUser& config,
               const std::shared_ptr<ApiManager>& apiManager,
               WatchStateLogger logger);
      virtual ~EmbyUser() = default;

      [[nodiscard]] bool GetValid() const;
      [[nodiscard]] std::string GetServerAndUserName() const;
      [[nodiscard]] std::string_view GetServerName() const;
      [[nodiscard]] std::string_view GetTypeAndServerName() const;
      [[nodiscard]] std::string_view GetUser() const;
      [[nodiscard]] const std::string& GetMediaPath() const;
      [[nodiscard]] std::optional<JellystatHistoryItems> GetWatchHistory();
      [[nodiscard]] std::optional<EmbyPlayState> GetPlayState(std::string_view id);

      void Update();

      struct PlexSyncState
      {
         const std::string& path;
         bool watched{false};
         int32_t playbackPercentage{0};
         int64_t timeWatchedEpoch{0};
      };
      void SyncStateWithPlex(const PlexSyncState& syncState, std::string& syncResults);

      struct EmbySyncState
      {
         const std::string& mediaPath;
         const std::string& path;
         bool watched{false};
         int32_t playbackPercentage{0};
         const std::string& timeWatched;
      };
      void SyncStateWithEmby(const EmbySyncState& syncState, std::string& syncResults);

   private:
      bool SyncPlexWatchedState(const std::string& plexPath);
      bool SyncPlexPlayState(const PlexSyncState& syncState);

      bool SyncEmbyWatchedState(std::string_view id);
      bool SyncEmbyPlayState(const EmbySyncState& syncState, std::string_view id);

      bool valid_{false};
      WatchStateLogger logger_;
      ServerUser config_;
      std::string userId_;
      std::string typeServerName_;

      EmbyApi* embyApi_{nullptr};
      JellystatApi* jellystatApi_{nullptr};
   };
}