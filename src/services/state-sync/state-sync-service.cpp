#include "state-sync-service.h"

#include "services/service-logger.h"
#include "services/service-types.h"
#include "services/service-utils.h"

#include <warp/api/api-tracearr.h>
#include <warp/log/log-utils.h>

#include <map>
#include <vector>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("State Sync");

      std::map<std::string, std::vector<const warp::TracearrHistoryItem*>> ConsolidateHistoryByUser(
         const std::vector<warp::TracearrHistoryItem>& items)
      {
         if (items.empty())
            return {};

         // Keep the newest history entry for each user/item pair, while preserving the
         // original grouping semantics that every user gets their own latest history list.
         std::map<std::pair<std::string, std::string>, const warp::TracearrHistoryItem*> latestByUserAndItem;
         for (const auto& item : items)
         {
            const auto key = std::make_pair(item.user.id, item.id);
            auto [it, inserted] = latestByUserAndItem.try_emplace(key, &item);
            if (!inserted && item.watchTime > it->second->watchTime)
               it->second = &item;
         }

         std::map<std::string, std::vector<const warp::TracearrHistoryItem*>> historyByUser;
         for (const auto& [userAndItem, item] : latestByUserAndItem)
            historyByUser[userAndItem.first].push_back(item);

         return historyByUser;
      }
   } // namespace

   StateSyncService::StateSyncService(const StateSyncConfig& config,
                                      std::shared_ptr<warp::ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_STATE_SYNC, apiManager, config.cron)
   {
      auto tracearrApi = apiManager->GetTracearrApi();
      if (!tracearrApi)
      {
         LogWarning("{} api not found. Required for {}", warp::GetFormattedTracearr(), warp::GetTag("service", SERVICE_NAME));
         return;
      }

      Init(config);
   }

   void StateSyncService::Init(const StateSyncConfig& config)
   {
      user_ = std::make_unique<StateSyncUser>(config.dryRun, GetApiManager(), ServiceLogger(*this));
   }

   void StateSyncService::Run()
   {
      auto tracearrApi = GetApiManager()->GetTracearrApi();
      if (!tracearrApi)
      {
         LogWarning(
            "{} api not found. Required for {}",
            warp::GetFormattedTracearr(),
            warp::GetTag("service", SERVICE_NAME));
         return;
      }

      // Get the watch history from tracearr for all servers and all users.
      constexpr uint32_t daysOfHistory{1};
      auto historyTime = GetIsoTimeStr(std::chrono::system_clock::now() - std::chrono::days(daysOfHistory));
      auto history = tracearrApi->GetWatchHistory(historyTime);
      if (!history)
      {
         return;
      }

      // Consolidate the history items to remove duplicates and keep the latest watch time for each item
      auto historyByUser = ConsolidateHistoryByUser(history->items);
      for (const auto& [userId, userHistory] : historyByUser)
      {
         try
         {
            user_->Sync(userId, userHistory);
         }
         catch (const std::exception& e)
         {
            LogWarning(
               "Encountered a problem syncing history for {}: {}",
               warp::GetTag("user", userHistory.front()->user.name),
               warp::GetTag("error", e.what()));
         }
      }
   }
} // namespace loomis
