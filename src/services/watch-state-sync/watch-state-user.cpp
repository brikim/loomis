#include "watch-state-user.h"

#include "logger/log-utils.h"
#include "services/service-utils.h"

#include <algorithm>
#include <ranges>

namespace loomis
{
   WatchStateUser::WatchStateUser(const UserSyncConfig& config,
                                  ApiManager* apiManager,
                                  const std::function<void(LogType, const std::string&)>& logFunc)
      : apiManager_(apiManager)
      , logFunc_(logFunc)
   {
      std::ranges::for_each(config.plex, [this, &apiManager, &logFunc](const auto& configPlexUser) {
         auto plexUser{std::make_unique<PlexUser>(configPlexUser,
                                                  apiManager,
                                                  logFunc)};
         if (plexUser->GetValid())
         {
            this->plexUsers_.emplace_back(std::move(plexUser));
         }
      });

      std::ranges::for_each(config.emby, [this, &apiManager, &logFunc](const auto& configEmbyUser) {
         auto embyUser{std::make_unique<EmbyUser>(configEmbyUser,
                                                  apiManager,
                                                  logFunc)};
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

   std::vector<const TautulliHistoryItem*> WatchStateUser::GetConsolodatedHistory(const TautulliHistoryItems& historyItems)
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

   void WatchStateUser::SyncPlexState(PlexUser* plexUser, std::string_view historyDate)
   {
      auto userHistory{plexUser->GetWatchHistory(historyDate)};
      if (!userHistory) return;

      auto consolodatedHistory = GetConsolodatedHistory(*userHistory);
      auto historyWithPaths = GetPlexPathsForHistoryItems(plexUser->GetServer(), consolodatedHistory);

      std::string syncServers;
      for (const auto* history : consolodatedHistory)
      {
         if (auto iter = historyWithPaths.find(history->id); iter != historyWithPaths.end())
         {
            for (auto& user : plexUsers_) user->SyncStateWithPlex(history, iter->second, syncServers);
            for (auto& user : embyUsers_) user->SyncStateWithPlex(history, iter->second, syncServers);
         }
      }
   }

   void WatchStateUser::Sync()
   {
      // Have all users update to the latest data
      UpdateAllUsers();

      constexpr uint32_t daysOfHistory{1};
      auto plexHistoryTime{GetDatetimeForHistoryPlex(daysOfHistory)};
      std::ranges::for_each(plexUsers_, [this, &plexHistoryTime](auto& plexUser) {
         this->SyncPlexState(plexUser.get(), plexHistoryTime);
      });
   }
}