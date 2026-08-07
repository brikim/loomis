#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-plex.h>
#include <warp/api/api-tautulli.h>
#include <warp/types.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace loomis
{
   class StateSyncPlexUser
   {
   public:
      StateSyncPlexUser(const ServerPlexUser& config,
                        bool dryRun,
                        const std::shared_ptr<warp::ApiManager>& apiManager,
                        ServiceLogger logger);
      ~StateSyncPlexUser() = default;

      [[nodiscard]] bool GetValid() const;
      [[nodiscard]] std::string GetServerAndUserName() const;
      [[nodiscard]] int32_t GetId() const;
      [[nodiscard]] std::string_view GetServerName() const;
      [[nodiscard]] std::optional<std::string> GetTracearrServerName() const;
      [[nodiscard]] std::string_view GetTypeAndServerName() const;
      [[nodiscard]] std::string_view GetUser() const;
      [[nodiscard]] std::optional<warp::TautulliHistoryItems> GetWatchHistory(std::string_view historyDate, int64_t epochHistoryTime);

      void Update();

      void SyncStateWithPlex();

      struct EmbySyncState
      {
         const warp::TracearrHistoryItem* item{nullptr};
         const std::filesystem::path& mediaPath;
         const std::filesystem::path& path;
      };
      void SyncStateWithEmby(const EmbySyncState& syncState, std::string& syncResults);

   private:
      std::optional<warp::PlexSearchResult> GetSyncStateItem(const EmbySyncState& syncState) const;
      bool SyncEmbyWatchedState(const EmbySyncState& syncState);
      bool SyncEmbyPlayState(const EmbySyncState& syncState);

      bool valid_{false};
      bool dryRun_{false};
      ServiceLogger logger_;
      ServerPlexUser config_;

      warp::PlexApi* api_{nullptr};
      warp::TautulliApi* trackerApi_{nullptr};

      warp::TautulliUserInfo userInfo_;
   };
}