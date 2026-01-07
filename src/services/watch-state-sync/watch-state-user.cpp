#include "watch-state-user.h"

#include "logger/log-utils.h"
#include "services/service-utils.h"

#include <algorithm>
#include <ranges>

namespace loomis
{
   WatchStateUser::WatchStateUser(const UserSyncConfig& config,
                                  ApiManager* apiManager,
                                  WatchStateLogger logger)
      : apiManager_(apiManager)
      , logger_(logger)
   {
      std::ranges::for_each(config.plex, [this, &apiManager](const auto& configPlexUser) {
         auto plexUser{std::make_unique<PlexUser>(configPlexUser,
                                                  apiManager,
                                                  logger_)};
         if (plexUser->GetValid())
         {
            this->plexUsers_.emplace_back(std::move(plexUser));
         }
      });

      std::ranges::for_each(config.emby, [this, &apiManager](const auto& configEmbyUser) {
         auto embyUser{std::make_unique<EmbyUser>(configEmbyUser,
                                                  apiManager,
                                                  logger_)};
         if (embyUser->GetValid())
         {
            this->embyUsers_.emplace_back(std::move(embyUser));
         }
      });

      if ((plexUsers_.size() + embyUsers_.size()) >= 2)
      {
         valid_ = true;
      }
   }

   bool WatchStateUser::GetValid() const
   {
      return valid_;
   }

   void WatchStateUser::UpdateAllUsers()
   {
      std::ranges::for_each(plexUsers_, [this](auto& plexUser) {
         plexUser->Update();
      });

      std::ranges::for_each(embyUsers_, [this](auto& embyUser) {
         embyUser->Update();
      });
   }

   std::vector<const TautulliHistoryItem*> WatchStateUser::GetConsolidatedHistory(const TautulliHistoryItems& historyItems)
   {
      if (historyItems.items.empty()) return {};

      std::vector<const TautulliHistoryItem*> consolidated;
      consolidated.reserve(historyItems.items.size());
      for (const auto& item : historyItems.items) consolidated.push_back(&item);

      // Sort using ranges and projections
      // 1st: sort by id, 2nd: sort by timeWatchedEpoch (descending)
      std::ranges::sort(consolidated, [](const auto& a, const auto& b) {
         if (a->id != b->id) return a->id < b->id;
         return a->timeWatchedEpoch > b->timeWatchedEpoch;
      });

      // Unique using ranges and projections
      auto [new_end, _] = std::ranges::unique(consolidated, std::ranges::equal_to{}, &TautulliHistoryItem::id);
      consolidated.erase(new_end, consolidated.end());

      return consolidated;
   }

   void WatchStateUser::LogSyncSummary(const LogSyncData& syncSummary)
   {
      if (syncSummary.watched)
      {
         logger_.LogInfo("{}:{} watched {} sync {} watch state",
                         syncSummary.server,
                         syncSummary.user,
                         syncSummary.name,
                         syncSummary.syncResults);
      }
      else
      {
         logger_.LogInfo("{}:{} played {}% of {} sync {} play state",
                         syncSummary.server,
                         syncSummary.user,
                         syncSummary.playbackPercentage,
                         syncSummary.name,
                         syncSummary.syncResults);
      }
   }

   std::unordered_map<int32_t, std::string> WatchStateUser::GetPlexPathsForHistoryItems(std::string_view server, const std::vector<const TautulliHistoryItem*> historyItems)
   {
      auto plexApi = apiManager_->GetPlexApi(server);

      // Guard Clause: Exit early if API is unavailable
      if (!plexApi || !plexApi->GetValid()) return {};

      std::vector<int32_t> ids;
      ids.reserve(historyItems.size());
      for (const auto* item : historyItems) ids.push_back(item->id);

      return plexApi->GetItemsPaths(ids);
   }

   void WatchStateUser::SyncPlexWatchState(const TautulliHistoryItem* item)
   {

   }

   void WatchStateUser::SyncPlexPlayState(const TautulliHistoryItem* item)
   {

   }

   void WatchStateUser::SyncPlexState(PlexUser& plexUser, std::string_view historyDate)
   {
      auto userHistory = plexUser.GetWatchHistory(historyDate);
      if (!userHistory || userHistory->items.empty()) return;

      auto consolidatedHistory = GetConsolidatedHistory(*userHistory);
      auto historyWithPaths = GetPlexPathsForHistoryItems(plexUser.GetServer(), consolidatedHistory);

      for (const auto* history : consolidatedHistory)
      {
         if (auto iter = historyWithPaths.find(history->id); iter != historyWithPaths.end())
         {
            std::string syncServers;

            auto plexSyncState = EmbyUser::PlexSyncState{
               .path = iter->second,
               .watched = history->watched,
               .playbackPercentage = history->playbackPercentage,
               .timeWatchedEpoch = history->timeWatchedEpoch};

            for (auto& user : plexUsers_)
               if (user->GetValid()) user->SyncStateWithPlex(history, iter->second, syncServers);
            for (auto& user : embyUsers_)
               if (user->GetValid()) user->SyncStateWithPlex(plexSyncState, syncServers);

            if (!syncServers.empty())
            {
               LogSyncSummary({
                  .server = plexUser.GetServerName(),
                  .user = plexUser.GetUser(),
                  .name = history->fullName,
                  .watched = history->watched,
                  .playbackPercentage = history->playbackPercentage,
                  .syncResults = syncServers
               });
            }
         }
      }
   }

   void WatchStateUser::SyncEmbyState(EmbyUser& embyUser)
   {
      auto userHistory = embyUser.GetWatchHistory();
      if (!userHistory || userHistory->items.empty()) return;
   }

   void WatchStateUser::Sync()
   {
      // Have all users update to the latest data
      UpdateAllUsers();

      constexpr uint32_t daysOfHistory{1};
      auto plexHistoryTime{GetDatetimeForHistoryPlex(daysOfHistory)};
      for (auto& plexUser : plexUsers_) SyncPlexState(*plexUser, plexHistoryTime);
      for (auto& embyUser : embyUsers_) SyncEmbyState(*embyUser);
   }
}