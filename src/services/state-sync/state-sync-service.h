#pragma once

#include "services/service-base.h"
#include "services/state-sync/state-sync-user.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-tracearr.h>

#include "config-reader/config-reader-types.h"

#include <memory>
#include <vector>

namespace loomis
{
   class StateSyncService : public ServiceBase
   {
   public:
      StateSyncService(const StateSyncConfig& config,
                       std::shared_ptr<warp::ApiManager> apiManager);
      ~StateSyncService() override = default;

      void Run() override;

   private:
      void Init(const StateSyncConfig& config);

      warp::TracearrApi* tracearrApi_{nullptr};
      std::unique_ptr<StateSyncUser> user_;
   };
} // namespace loomis
