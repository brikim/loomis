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
      std::ranges::for_each(config.plex, [this, dryRun, &config](const auto& configPlexUser) {
         auto plexUser{std::make_unique<StateSyncPlexUser>(configPlexUser,
                                                           config.tracearr.userName,
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
      // Update all emby users to current data.
      std::ranges::for_each(embyUsers_, [this](auto& embyUser) {
         embyUser->Update();
      });
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

      // This will hold the list of servers that were synced for this item. It will be used to log the summary of the sync.
      std::string syncServers;

      // Sync the plex play state with all plex users.
      for (auto& user : plexUsers_)
         if (user->GetValid())
            user->SyncStateWithPlex();

      // Sync the plex play state with all emby users.
      for (auto& user : embyUsers_)
         if (user->GetValid())
            user->SyncStateWithPlex(historyItem, itemPath.value(), syncServers);

      // If syncServers is not empty, then log the summary of the sync.
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

      // This will hold the list of servers that were synced for this item. It will be used to log the summary of the sync.
      std::string syncServers;

      // Sync the emby play state with all plex users.
      auto plexSyncState = StateSyncPlexUser::EmbySyncState{
        .item = historyItem,
        .mediaPath = embyUser.GetMediaPath(),
        .path = itemPath.value(),
      };
      for (auto& user : plexUsers_)
         if (user->GetValid())
            user->SyncStateWithEmby(plexSyncState, syncServers);

      // Sync the emby play state with all other emby users. Ignore the current user since they are already in sync with tracearr.
      auto embySyncState = StateSyncEmbyUser::EmbySyncState{
         .item = historyItem,
         .mediaPath = embyUser.GetMediaPath(),
         .path = itemPath.value()
      };
      for (auto& user : embyUsers_)
         if (user->GetServerName() != embyUser.GetServerName() && user->GetValid())
            user->SyncStateWithEmby(embySyncState, syncServers);

      // If syncServers is not empty, then log the summary of the sync.
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

      // Create a view of the history items that are only for this user
      auto isUser = [this](auto* item) { return tracearrUserName_ == item->user; };
      auto userViewItems = historyItems | std::views::filter(isUser);

      // Sync each item with the associated servers and users assigned to this user
      for (const auto* item : userViewItems)
      {
         if (item->serverType == warp::TracearrServerType::PLEX)
         {
            for (auto& plexUser : plexUsers_)
            {
               if (plexUser->GetValid() && plexUser->GetTracearrServerName() == item->serverName)
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
               if (embyUser->GetValid() && embyUser->GetTracearrServerName() == item->serverName)
               {
                  SyncEmbyState(item, *embyUser);
                  break;
               }
            }
         }
      }
   }
}