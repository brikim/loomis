#pragma once

#include "api/api-manager.h"
#include "config-reader/config-reader-types.h"
#include "services/service-base.h"
#include "types.h"

#include <optional>
#include <string>

namespace loomis
{
   class WatchStateSyncService : public ServiceBase
   {
   public:
      WatchStateSyncService(const WatchStateSyncConfig& config,
                          std::shared_ptr<ApiManager> apiManager);
      virtual ~WatchStateSyncService() = default;

      void Run() override;

   private:
      bool CheckValidUser(ApiType type, ApiType trackerType, const ServerUser& serverUser);

      void Init(const WatchStateSyncConfig& config);

      std::vector<UserSyncConfig> watchStateUsers_;
   };
}