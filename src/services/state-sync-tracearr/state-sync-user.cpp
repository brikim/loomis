#include "state-sync-user.h"

#include "services/service-utils.h"

#include <warp/log/log-utils.h>

#include <algorithm>
#include <ranges>

namespace loomis
{
   StateSyncUser::StateSyncUser(const UserSyncTracearrConfig& config,
                                  std::shared_ptr<warp::ApiManager> apiManager,
                                  ServiceLogger logger)
      : apiManager_(std::move(apiManager))
      , logger_(logger)
      , tracearrUserName_(config.tracearr.userName)
   {
      std::ranges::for_each(config.plex, [this](const auto& configPlexUser) {
         auto plexUser{std::make_unique<PlexUser>(configPlexUser,
                                                  apiManager_,
                                                  logger_)};
         if (plexUser->GetValid())
         {
            this->plexUsers_.emplace_back(std::move(plexUser));
         }
      });

      std::ranges::for_each(config.emby, [this](const auto& configEmbyUser) {
         auto embyUser{std::make_unique<EmbyUser>(configEmbyUser,
                                                  apiManager_,
                                                  logger_)};
         if (embyUser->GetValid())
         {
            this->embyUsers_.emplace_back(std::move(embyUser));
         }
      });

      if (!tracearrUserName_.empty() && (plexUsers_.size() + embyUsers_.size()) >= 2)
      {
         valid_ = true;
      }
   }

   bool StateSyncUser::GetValid() const
   {
      return valid_;
   }

   std::string StateSyncUser::GetServerAndUserName() const
   {
      std::string names;
      for (const auto& plexUser : plexUsers_)
      {
         if (!names.empty())
            names += ", ";
         names += plexUser->GetServerAndUserName();
      }
      for (const auto& embyUser : embyUsers_)
      {
         if (!names.empty())
            names += ", ";
         names += embyUser->GetServerAndUserName();
      }
      return names;
   }

   void StateSyncUser::UpdateAllUsers()
   {
      std::ranges::for_each(plexUsers_, [this](auto& plexUser) {
         plexUser->Update();
      });

      std::ranges::for_each(embyUsers_, [this](auto& embyUser) {
         embyUser->Update();
      });
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

   std::vector<const warp::TautulliHistoryItem*> StateSyncUser::GetConsolidatedPlexHistory(const warp::TautulliHistoryItems& historyItems)
   {
      return ConsolidateHistory(historyItems.items, [](const auto* i) { return i->timeWatchedEpoch; });
   }

   std::vector<const warp::JellystatHistoryItem*> StateSyncUser::GetConsolidatedEmbyHistory(const warp::JellystatHistoryItems& historyItems)
   {
      return ConsolidateHistory(historyItems.items, [](const auto* i) { return i->watchTime; });
   }

   void StateSyncUser::LogSyncSummary(const LogSyncData& syncSummary)
   {
      if (syncSummary.watched)
      {
         logger_.LogInfo("{}:{} watched {} sync {} watch state",
                         syncSummary.server,
                         syncSummary.user,
                         warp::GetStandoutText(syncSummary.name),
                         syncSummary.syncResults);
      }
      else
      {
         logger_.LogInfo("{}:{} played {}% of {} sync {} play state",
                         syncSummary.server,
                         syncSummary.user,
                         syncSummary.playbackPercentage,
                         warp::GetStandoutText(syncSummary.name),
                         syncSummary.syncResults);
      }
   }

   std::unordered_map<std::string, std::filesystem::path> StateSyncUser::GetPlexPathsForHistoryItems(std::string_view server,
                                                                                                      const std::vector<const warp::TautulliHistoryItem*> historyItems)
   {
      auto plexApi = apiManager_->GetPlexApi(server);

      if (!plexApi || !plexApi->GetValid())
         return {};

      std::vector<std::string> ids;
      ids.reserve(historyItems.size());
      for (const auto* item : historyItems)
      {
         if (item->live) continue;
         ids.push_back(item->id);
      }

      if (ids.empty())
      {
         return {};
      }
      return plexApi->GetItemsPaths(ids);
   }

   void StateSyncUser::SyncPlexState(PlexUser& plexUser, std::string_view historyDate, int64_t epochHistoryTime)
   {
      auto userHistory = plexUser.GetWatchHistory(historyDate, epochHistoryTime);
      if (!userHistory || userHistory->items.empty())
         return;

      auto consolidatedHistory = GetConsolidatedPlexHistory(*userHistory);
      auto historyWithPaths = GetPlexPathsForHistoryItems(plexUser.GetServerName(), consolidatedHistory);

      for (const auto* history : consolidatedHistory)
      {
         auto iter = historyWithPaths.find(history->id);
         if (iter == historyWithPaths.end())
            return;

         std::string syncServers;

         auto plexSyncState = EmbyUser::PlexSyncState{
            .path = iter->second,
            .watched = history->watched,
            .playbackPercentage = history->playbackPercentage,
            .timeWatchedEpoch = history->timeWatchedEpoch};

         for (auto& user : plexUsers_)
            if (user->GetValid())
               user->SyncStateWithPlex();

         for (auto& user : embyUsers_)
            if (user->GetValid())
               user->SyncStateWithPlex(plexSyncState, syncServers);

         if (!syncServers.empty())
         {
            LogSyncSummary({
               .server = plexUser.GetTypeAndServerName(),
               .user = plexUser.GetUser(),
               .name = history->fullName,
               .watched = history->watched,
               .playbackPercentage = history->playbackPercentage,
               .syncResults = syncServers
            });
         }
      }
   }

   void StateSyncUser::SyncEmbyState(EmbyUser& embyUser)
   {
      auto userHistory = embyUser.GetWatchHistory();
      if (!userHistory || userHistory->items.empty())
         return;

      const auto cutoff = GetIsoTimeStr(std::chrono::system_clock::now() - std::chrono::days(1));

      // Remove all items older than 24 hours
      std::erase_if(userHistory->items, [&cutoff](const auto& item) {
         return item.watchTime < cutoff;
      });

      auto consolidatedHistory = GetConsolidatedEmbyHistory(*userHistory);
      for (auto& item : consolidatedHistory)
      {
         std::string syncServers;
         auto playState = embyUser.GetPlayState(item->episodeId.has_value() ? *item->episodeId : item->id);

         if (!playState)
            continue;

         auto plexSyncState = PlexUser::EmbySyncState{
            .name = item->name,
            .mediaPath = embyUser.GetMediaPath(),
            .path = playState->path,
            .watched = playState->watched,
            .playbackPercentage = static_cast<int32_t>(std::lround(playState->percentage)),
            .timeWatched = item->watchTime
         };

         auto embySyncState = EmbyUser::EmbySyncState{
            .mediaPath = embyUser.GetMediaPath(),
            .path = playState->path,
            .watched = playState->watched,
            .playbackPercentage = static_cast<int32_t>(std::lround(playState->percentage)),
            .timeWatched = item->watchTime
         };

         for (auto& user : plexUsers_)
            if (user->GetValid())
               user->SyncStateWithEmby(plexSyncState, syncServers);

         for (auto& user : embyUsers_)
            if (user->GetServerName() != embyUser.GetServerName() && user->GetValid())
               user->SyncStateWithEmby(embySyncState, syncServers);

         if (!syncServers.empty())
         {
            LogSyncSummary({
               .server = embyUser.GetTypeAndServerName(),
               .user = embyUser.GetUser(),
               .name = item->seriesName ? std::format("{} - {}", *item->seriesName, item->name) : item->name,
               .watched = playState->watched,
               .playbackPercentage = static_cast<int32_t>(std::lround(playState->percentage)),
               .syncResults = syncServers
            });
         }
      }
   }

   void StateSyncUser::Sync(const std::vector<const warp::TracearrHistoryItem*>& historyItems)
   {
      // Have all users update to the latest data
      UpdateAllUsers();

      constexpr uint32_t daysOfHistory{1};
      auto plexHistoryTime = GetDatetimeForHistoryPlex(daysOfHistory);
      auto plexEpochHistoryTime = GetEpochTimeForPlexHistory(daysOfHistory);
      for (auto& plexUser : plexUsers_)
         SyncPlexState(*plexUser, plexHistoryTime, plexEpochHistoryTime);

      for (auto& embyUser : embyUsers_)
         SyncEmbyState(*embyUser);
   }
}