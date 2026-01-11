#include "watch-state-sync-service.h"

#include "logger/log-utils.h"
#include "services/watch-state-sync/watch-state-logger.h"

#include <algorithm>
#include <ranges>

namespace loomis
{
   WatchStateSyncService::WatchStateSyncService(const WatchStateSyncConfig& config,
                                            std::shared_ptr<ApiManager> apiManager)
      : ServiceBase("Watch State Sync", log::ANSI_CODE_SERVICE_WATCH_STATE_SYNC, apiManager, config.cron)
   {
      Init(config);
   }

   void WatchStateSyncService::Init(const WatchStateSyncConfig& config)
   {
      for (const auto& user : config.users)
      {
         auto watchStateUser{std::make_unique<WatchStateUser>(user, GetApiManager(), WatchStateLogger(*this))};
         if (watchStateUser->GetValid())
         {
            users_.emplace_back(std::move(watchStateUser));
         }
      }
   }

   void WatchStateSyncService::Run()
   {
      std::ranges::for_each(users_, [&](auto& user) {
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
      });
   }
}