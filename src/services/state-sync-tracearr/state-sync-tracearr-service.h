#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-base.h"

#include <warp/api//api-manager.h>

#include <memory>
#include <vector>

namespace loomis
{
   class StateSyncTracearrService : public ServiceBase
   {
   public:
      StateSyncTracearrService(const StateSyncTracearrConfig& config,
                               std::shared_ptr<warp::ApiManager> apiManager);
      ~StateSyncTracearrService() = default;

      void Run() override;

   private:
      void Init(const StateSyncTracearrConfig& config);
   };
}