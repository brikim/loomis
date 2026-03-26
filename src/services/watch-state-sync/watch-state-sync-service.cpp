#include "watch-state-sync-service.h"

#include "services/service-logger.h"
#include "services/service-types.h"

#include <warp/log/log-utils.h>

#include <algorithm>
#include <ranges>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("Watch State Sync");
   }

   WatchStateSyncService::WatchStateSyncService(const WatchStateSyncConfig& config,
                                                std::shared_ptr<warp::ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_WATCH_STATE_SYNC, apiManager, config.cron)
   {
      Init(config);
   }

   void WatchStateSyncService::Init(const WatchStateSyncConfig& config)
   {
      for (const auto& user : config.users)
      {
         if (auto watchStateUser{std::make_unique<WatchStateUser>(user, GetApiManager(), ServiceLogger(*this))};
             watchStateUser->GetValid())
         {
            users_.emplace_back(std::move(watchStateUser));
         }
      }
   }

   void WatchStateSyncService::Run()
   {
      for (auto& user : users_)
      {
         try
         {
            user->Sync();
         }
         catch (const std::exception& e)
         {
            LogWarning("Encountered a error for {} during sync: {}",
                       user->GetServerAndUserName(),
                       e.what());
         }
      };
   }
}