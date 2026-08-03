#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-base.h"
#include "services/state-sync-tracearr/state-sync-user.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-tracearr.h>

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

      std::vector<const warp::TracearrHistoryItem*> GetConsolidatedHistory(const warp::TracearrHistoryItems& historyItems);

      warp::TracearrApi* tracearrApi_{nullptr};
      std::vector<std::unique_ptr<StateSyncUser>> users_;
   };
}