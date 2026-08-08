#include "state-sync-service.h"

#include "services/service-logger.h"
#include "services/service-types.h"
#include "services/service-utils.h"

#include <warp/log/log-utils.h>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("State Sync");
   }

   StateSyncService::StateSyncService(const StateSyncConfig& config,
                                      std::shared_ptr<warp::ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_STATE_SYNC, apiManager, config.cron)
      , tracearrApi_(apiManager->GetTracearrApi())
   {
      if (!tracearrApi_)
      {
         LogWarning("{} api not found. Required for service StateSyncService", warp::GetFormattedTracearr());
         return;
      }

      Init(config);
   }

   void StateSyncService::Init(const StateSyncConfig& config)
   {
      for (const auto& user : config.users)
      {
         if (auto stateSyncUser{std::make_unique<StateSyncUser>(user, config.dryRun, GetApiManager(), ServiceLogger(*this))};
             stateSyncUser->GetValid())
         {
            users_.emplace_back(std::move(stateSyncUser));
         }
      }
   }

   template <typename T, typename TimeFieldProj>
   std::vector<const T*> ConsolidateHistory(const std::vector<T>& items, TimeFieldProj timeProj)
   {
      if (items.empty()) return {};

      std::vector<const T*> consolidated;
      consolidated.reserve(items.size());
      for (const auto& item : items)
         consolidated.push_back(&item);

      // Sort by ID, then by Time (descending)
      std::ranges::sort(consolidated, [&](const auto* a, const auto* b) {
         if (a->id != b->id) return a->id < b->id;
         return timeProj(a) > timeProj(b);
      });

      // Unique based on ID
      auto [new_end, _] = std::ranges::unique(consolidated, std::ranges::equal_to{}, &T::id);
      consolidated.erase(new_end, consolidated.end());

      return consolidated;
   }

   void StateSyncService::Run()
   {
      // Get the watch history from tracearr for all servers and all users.
      constexpr uint32_t daysOfHistory{1};
      auto historyTime = GetIsoTimeStr(std::chrono::system_clock::now() - std::chrono::days(daysOfHistory));
      auto history = tracearrApi_->GetWatchHistory(historyTime);
      if (!history)
      {
         return;
      }

      // Consolidate the history items to remove duplicates and keep the latest watch time for each item
      auto consolidatedHistory = ConsolidateHistory(history->items, [](const auto* i) { return i->watchTime; });

      // Iterate over all users passing in the history to check for needed state syncs.
      for (auto& user : users_)
      {
         try
         {
            user->Sync(consolidatedHistory);
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