#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-manager.h>
#include <warp/api/api-tracearr-types.h>
#include <warp/types.h>

#include <chrono>
#include <filesystem>
#include <functional>

namespace loomis
{
   class StateSyncEmbyUser
   {
   public:
      StateSyncEmbyUser(const ServerUser& config,
                        bool dryRun,
                        const std::shared_ptr<warp::ApiManager>& apiManager,
                        ServiceLogger logger);
      ~StateSyncEmbyUser() = default;

      [[nodiscard]] bool GetValid() const;
      [[nodiscard]] std::string GetServerAndUserName() const;
      [[nodiscard]] std::string_view GetServerName() const;
      [[nodiscard]] std::optional<std::string> GetTracearrServerName() const;
      [[nodiscard]] std::string_view GetTypeAndServerName() const;
      [[nodiscard]] std::string_view GetUser() const;
      [[nodiscard]] const std::filesystem::path& GetMediaPath() const;
      [[nodiscard]] std::optional<warp::EmbyPlayState> GetPlayState(std::string_view id);

      void Update();

      void SyncStateWithPlex(const warp::TracearrHistoryItem* item,
                             const std::filesystem::path& itemPath,
                             std::string& syncResults);

      struct EmbySyncState
      {
         const warp::TracearrHistoryItem* item{nullptr};
         const std::filesystem::path& mediaPath;
         const std::filesystem::path& path;
      };
      void SyncStateWithEmby(const EmbySyncState& syncState, std::string& syncResults);

   private:
      bool SyncPlexWatchedState(std::string_view embyId, const warp::TracearrHistoryItem* historyItem);
      bool SyncPlexPlayState(std::string_view embyId, const warp::TracearrHistoryItem* historyItem);

      bool SyncEmbyWatchedState(std::string_view id);
      bool SyncEmbyPlayState(const EmbySyncState& syncState, std::string_view id);

      bool valid_{false};
      ServiceLogger logger_;
      bool dryRun_{false};
      ServerUser config_;
      std::string userId_;

      warp::EmbyApi* embyApi_{nullptr};
   };
}