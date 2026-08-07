#include "state-sync-user.h"

#include "services/service-utils.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-plex.h>
#include <warp/api/api-tracearr.h>
#include <warp/log/log-utils.h>

#include <algorithm>
#include <ranges>

namespace loomis
{
   StateSyncUser::StateSyncUser(const UserSyncTracearrConfig& config,
                                bool dryRun,
                                std::shared_ptr<warp::ApiManager> apiManager,
                                ServiceLogger logger)
      : apiManager_(std::move(apiManager))
      , logger_(logger)
      , dryRunText_(dryRun ? "[DRY RUN] " : "")
      , tracearrUserName_(config.tracearr.userName)
   {
      std::ranges::for_each(config.plex, [this, dryRun](const auto& configPlexUser) {
         auto plexUser{std::make_unique<StateSyncPlexUser>(configPlexUser,
                                                           dryRun,
                                                           apiManager_,
                                                           logger_)};
         if (plexUser->GetValid())
         {
            this->plexUsers_.emplace_back(std::move(plexUser));
         }
      });

      std::ranges::for_each(config.emby, [this, dryRun](const auto& configEmbyUser) {
         auto embyUser{std::make_unique<StateSyncEmbyUser>(configEmbyUser,
                                                           dryRun,
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
         logger_.LogInfo("{}{}:{} watched {} sync {} watch state",
                         dryRunText_,
                         syncSummary.server,
                         syncSummary.user,
                         warp::GetStandoutText(syncSummary.name),
                         syncSummary.syncResults);
      }
      else
      {
         logger_.LogInfo("{}{}:{} played {}% of {} sync {} play state",
                         dryRunText_,
                         syncSummary.server,
                         syncSummary.user,
                         syncSummary.playbackPercentage,
                         warp::GetStandoutText(syncSummary.name),
                         syncSummary.syncResults);
      }
   }

   void StateSyncUser::SyncPlexState(const warp::TracearrHistoryItem* historyItem, StateSyncPlexUser& plexUser)
   {
      auto plexApi = apiManager_->GetPlexApi(plexUser.GetServerName());
      if (!plexApi || !plexApi->GetValid())
         return;

      auto itemPath = plexApi->GetItemPath(historyItem->serverRatingKey);
      if (!itemPath)
         return;

      std::string syncServers;

      for (auto& user : plexUsers_)
         if (user->GetValid())
            user->SyncStateWithPlex();

      for (auto& user : embyUsers_)
         if (user->GetValid())
            user->SyncStateWithPlex(historyItem, itemPath.value(), syncServers);

      if (!syncServers.empty())
      {
         LogSyncSummary({
            .server = plexUser.GetTypeAndServerName(),
            .user = plexUser.GetUser(),
            .name = historyItem->fullName,
            .watched = historyItem->watched,
            .playbackPercentage = historyItem->playbackPercentage,
            .syncResults = syncServers
         });
      }
   }

   void StateSyncUser::SyncEmbyState(const warp::TracearrHistoryItem* historyItem, StateSyncEmbyUser& embyUser)
   {
      auto embyApi = apiManager_->GetEmbyApi(embyUser.GetServerName());
      if (!embyApi || !embyApi->GetValid())
         return;

      auto itemPath = embyApi->GetItemPath(historyItem->serverRatingKey);
      if (!itemPath)
         return;

      auto plexSyncState = StateSyncPlexUser::EmbySyncState{
         .item = historyItem,
         .mediaPath = embyUser.GetMediaPath(),
         .path = itemPath.value(),
      };

      auto embySyncState = StateSyncEmbyUser::EmbySyncState{
         .item = historyItem,
         .mediaPath = embyUser.GetMediaPath(),
         .path = itemPath.value()
      };

      std::string syncServers;

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
            .name = historyItem->fullName,
            .watched = historyItem->watched,
            .playbackPercentage = static_cast<int32_t>(std::lround(historyItem->playbackPercentage)),
            .syncResults = syncServers
         });
      }
   }

   void StateSyncUser::Sync(const std::vector<const warp::TracearrHistoryItem*>& historyItems)
   {
      auto tracearrApi = apiManager_->GetTracearrApi();

      // Have all users update to the latest data
      UpdateAllUsers();

      auto isUser = [this](auto* item) { return tracearrUserName_ == item->user; };
      auto userViewItems = historyItems | std::views::filter(isUser);
      for (const auto* item : userViewItems)
      {
         if (item->serverType == warp::TracearrServerType::PLEX)
         {
            for (auto& plexUser : plexUsers_)
            {
               if (plexUser->GetTracearrServerName() == item->serverName)
               {
                  SyncPlexState(item, *plexUser);
                  break;
               }
            }
         }
         else if (item->serverType == warp::TracearrServerType::EMBY)
         {
            for (auto& embyUser : embyUsers_)
            {
               if (embyUser->GetTracearrServerName() == item->serverName)
               {
                  SyncEmbyState(item, *embyUser);
                  break;
               }
            }
         }
      }
   }
}