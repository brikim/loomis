#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-plex.h>
#include <warp/api/api-tautulli.h>
#include <warp/types.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace loomis
{
   class StateSyncPlexUser
   {
   public:
      StateSyncPlexUser(const ServerPlexUser& config,
                        std::string_view tracearrUserName,
                        bool dryRun,
                        const std::shared_ptr<warp::ApiManager>& apiManager,
                        ServiceLogger logger);
      ~StateSyncPlexUser() = default;

      [[nodiscard]] bool GetValid() const;
      [[nodiscard]] std::string GetServerAndUserName() const;
      [[nodiscard]] std::string_view GetServerName() const;
      [[nodiscard]] std::optional<std::string> GetTracearrServerName() const;
      [[nodiscard]] std::string_view GetTypeAndServerName() const;
      [[nodiscard]] std::string_view GetUser() const;
      [[nodiscard]] const std::filesystem::path& GetMediaPath() const;

      struct PlexSyncState
      {
         const warp::TracearrHistoryItem* item{nullptr};
         const std::filesystem::path& mediaPath;
         const std::filesystem::path& path;
      };
      void SyncState(const PlexSyncState& syncState, std::string& syncResults);

   private:
      std::optional<warp::PlexSearchResult> GetSyncStateItem(const PlexSyncState& syncState) const;
      bool SyncWatchedState(const PlexSyncState& syncState);
      bool SyncPlayState(const PlexSyncState& syncState);

      bool valid_{false};
      bool dryRun_{false};
      std::string tracearrUserName_;
      ServiceLogger logger_;
      ServerPlexUser config_;

      warp::PlexApi* api_{nullptr};
   };
}