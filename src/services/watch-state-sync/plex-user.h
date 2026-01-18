#pragma once

#include "api/api-manager.h"
#include "api/api-plex.h"
#include "api/api-tautulli.h"
#include "config-reader/config-reader-types.h"
#include "services/watch-state-sync/watch-state-logger.h"
#include "types.h"

#include <functional>
#include <optional>
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
      [[nodiscard]] std::optional<TautulliHistoryItems> GetWatchHistory(std::string_view historyDate, int64_t epochHistoryTime);

      void Update();

      void SyncStateWithPlex();

      struct EmbySyncState
      {
         std::string_view name;
         std::string_view mediaPath;
         std::string_view path;
         bool watched{false};
         int32_t playbackPercentage{0};
         std::string_view timeWatched;
      };
      void SyncStateWithEmby(const EmbySyncState& syncState, std::string& syncResults);

   private:
      std::optional<PlexSearchResult> GetSyncStateItem(const EmbySyncState& syncState) const;
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