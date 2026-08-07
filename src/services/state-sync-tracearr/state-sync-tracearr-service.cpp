#include "state-sync-tracearr-service.h"

#include "services/service-logger.h"
#include "services/service-types.h"
#include "services/service-utils.h"

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
      , tracearrApi_(apiManager->GetTracearrApi())
   {
      if (!tracearrApi_)
      {
         LogWarning("{} api not found", warp::GetFormattedTracearr());
         return;
      }

      Init(config);
   }

   void StateSyncTracearrService::Init(const StateSyncTracearrConfig& config)
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

   void StateSyncTracearrService::Run()
   {
      constexpr uint32_t daysOfHistory{1};
      auto historyTime = GetIsoTimeStr(std::chrono::system_clock::now() - std::chrono::days(daysOfHistory));
      auto history = tracearrApi_->GetWatchHistory(historyTime);
      if (!history)
      {
         return;
      }

      auto consolidatedHistory = ConsolidateHistory(history->items, [](const auto* i) { return i->watchTime; });

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