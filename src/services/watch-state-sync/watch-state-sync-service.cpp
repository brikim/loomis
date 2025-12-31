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

   bool WatchStateSyncService::CheckValidUser(ApiType type, ApiType trackerType, const ServerUser& serverUser)
   {
      auto* api{GetApiManager()->GetApi(type, serverUser.server)};
      auto* trackerApi{GetApiManager()->GetApi(trackerType, serverUser.server)};
      if (api && trackerApi)
      {
         return true;
      }
      else
      {
         if (!api)
         {
            LogWarning(std::format("{} api not found for {}",
                                   utils::GetServerName(utils::GetFormattedApiName(type), serverUser.server),
                                   utils::GetTag("user", serverUser.user)));
         }

         if (!trackerApi)
         {
            LogWarning(std::format("{} tracker api not found for {}. Required for this service.",
                                   utils::GetServerName(utils::GetFormattedApiName(trackerType), serverUser.server),
                                   utils::GetTag("user", serverUser.user)));
         }
      }
      return false;
   }

   void WatchStateSyncService::Init(const WatchStateSyncConfig& config)
   {
      for (const auto& configUser : config.userSyncs)
      {
         UserSyncConfig watchStateUser;

         std::ranges::for_each(configUser.plex, [this, &watchStateUser](const auto& configPlexUser) {
            if (this->CheckValidUser(ApiType::PLEX, ApiType::TAUTULLI, configPlexUser))
            {
               watchStateUser.plex.emplace_back(configPlexUser);
            }
         });

         std::ranges::for_each(configUser.emby, [this, &watchStateUser](const auto& configEmbyUser) {
            if (this->CheckValidUser(ApiType::EMBY, ApiType::JELLYSTAT, configEmbyUser))
            {
               watchStateUser.emby.emplace_back(configEmbyUser);
            }
         });

         if ((watchStateUser.plex.size() + watchStateUser.emby.size()) >= 2)
         {
            watchStateUsers_.emplace_back(watchStateUser);
         }
      }
   }

   void WatchStateSyncService::Run()
   {

   }
}