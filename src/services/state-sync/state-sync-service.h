#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-base.h"
#include "services/state-sync/state-sync-user.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-tracearr.h>

#include <memory>
#include <vector>

namespace loomis
{
   class StateSyncService : public ServiceBase
   {
   public:
      StateSyncService(const StateSyncConfig& config,
                               std::shared_ptr<warp::ApiManager> apiManager);
      ~StateSyncService() = default;

      void Run() override;

   private:
      void Init(const StateSyncConfig& config);

      warp::TracearrApi* tracearrApi_{nullptr};
      std::vector<std::unique_ptr<StateSyncUser>> users_;
   };
}