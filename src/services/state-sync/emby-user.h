#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-manager.h>
#include <warp/api/api-tracearr-types.h>
#include <warp/types.h>

#include <filesystem>
#include <string>
#include <string_view>

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

      struct EmbySyncState
      {
         const warp::TracearrHistoryItem* item{nullptr};
         const std::filesystem::path& mediaPath;
         const std::filesystem::path& path;
      };
      void SyncState(const EmbySyncState& syncState, std::string& syncResults);

   private:
      bool SyncWatchedState(std::string_view id);
      bool SyncPlayState(const EmbySyncState& syncState, std::string_view id);

      bool valid_{false};
      ServiceLogger logger_;
      bool dryRun_{false};
      ServerUser config_;
      std::string userId_;

      warp::EmbyApi* api_{nullptr};
   };
}