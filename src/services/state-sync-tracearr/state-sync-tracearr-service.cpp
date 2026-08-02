#include "state-sync-tracearr-service.h"

#include "services/service-logger.h"
#include "services/service-types.h"

#include <warp/log/log-utils.h>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("State Sync Tracearr");
   }

   StateSyncTracearrService::StateSyncTracearrService(const StateSyncTracearrConfig& config,
                                                std::shared_ptr<warp::ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_STATE_SYNC_TRACEARR, apiManager, config.cron)
   {
      Init(config);
   }

   void StateSyncTracearrService::Init(const StateSyncTracearrConfig& config)
   {

   }

   void StateSyncTracearrService::Run()
   {

   }
}