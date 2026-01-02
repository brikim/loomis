#include "watch-state-sync-service.h"

#include "logger/log-utils.h"

#include <algorithm>
#include <format>
#include <ranges>

namespace loomis
{
   WatchStateSyncService::WatchStateSyncService(const WatchStateSyncConfig& config,
                                            std::shared_ptr<ApiManager> apiManager)
      : ServiceBase("Watch State Sync", utils::ANSI_CODE_SERVICE_WATCH_STATE_SYNC, apiManager, config.cron)
   {
      Init(config);
   }

   void WatchStateSyncService::Init(const WatchStateSyncConfig& config)
   {
      for (const auto& configUser : config.userSyncs)
      {
         auto watchStateUser{std::make_unique<WatchStateUser>(configUser,
                                                              GetApiManager(),
                                                              [this](LogType type, const std::string& msg) { this->Log(type, msg); })};
         if (watchStateUser->GetValid())
         {
            users_.emplace_back(std::move(watchStateUser));
         }
      }
   }

   void WatchStateSyncService::Run()
   {
      std::ranges::for_each(users_, [](auto& user) {
         user->Sync();
      });
   }
}