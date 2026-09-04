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
   StateSyncUser::StateSyncUser(bool dryRun,
                                std::shared_ptr<warp::ApiManager> apiManager,
                                ServiceLogger logger)
      : apiManager_(std::move(apiManager))
      , logger_(logger)
      , dryRunText_(dryRun ? "[DRY RUN] " : "")
      , tracearrApi_(apiManager_->GetTracearrApi())
   {
      plexUser_ = std::make_unique<StateSyncPlexUser>(dryRun, logger_);
      embyUser_ = std::make_unique<StateSyncEmbyUser>(dryRun, logger_);
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

   void StateSyncUser::SyncPlexState(const warp::TracearrUser& user, const warp::TracearrHistoryItem& historyItem)
   {
      auto plexApi = apiManager_->GetTracearrPlexApi(historyItem.serverName);
      if (!plexApi || !plexApi->GetValid())
         return;

      auto itemPath = plexApi->GetItemPath(historyItem.serverRatingKey);
      if (!itemPath)
         return;


      // This will hold the list of servers that were synced for this item. It will be used to log the summary of the sync.
      std::string syncServers;
      std::string_view plexUserName;

      for (auto& account : user.accounts)
      {
         // If this is the server that watched the item get the server user name and skip syncing since it is already in sync
         if (account.serverId == historyItem.serverId)
         {
            plexUserName = account.externalUserName;
            continue;
         }

         if (account.serverType == warp::TracearrServerType::PLEX)
         {
            // Syncing plex to plex servers is not currently supported
         }
         else if (account.serverType == warp::TracearrServerType::EMBY)
         {
            auto serverName = tracearrApi_->GetServerNameFromId(account.serverId);
            if (!serverName.has_value())
               continue;

            auto* embyApi = apiManager_->GetTracearrEmbyApi(serverName.value());
            if (!embyApi)
               continue;

            StateSyncEmbyUser::EmbySyncState syncState{
               .item = historyItem,
               .mediaPath = ReplaceMediaPath(itemPath.value(), plexApi->GetMediaPath(), embyApi->GetMediaPath()),
               .embyApi = embyApi,
               .embyUserId = account.externalUserId
            };
            embyUser_->SyncState(syncState, syncServers);
         }
      }

      // If syncServers is not empty, then log the summary of the sync.
      if (!syncServers.empty())
      {
         LogSyncSummary({
            .server = plexApi->GetPrettyName(),
            .user = plexUserName,
            .name = historyItem.fullName,
            .watched = historyItem.watched,
            .playbackPercentage = historyItem.playbackPercentage.has_value() ? historyItem.playbackPercentage.value() : 0,
            .syncResults = syncServers
         });
      }
   }

   void StateSyncUser::SyncEmbyState(const warp::TracearrUser& user, const warp::TracearrHistoryItem& historyItem)
   {
      auto embyApi = apiManager_->GetTracearrEmbyApi(historyItem.serverName);
      if (!embyApi || !embyApi->GetValid())
         return;

      auto itemPath = embyApi->GetItemPath(historyItem.serverRatingKey);
      if (!itemPath)
         return;

      // This will hold the list of servers that were synced for this item. It will be used to log the summary of the sync.
      std::string syncServers;
      std::string_view embyUserName;

      for (auto& account : user.accounts)
      {
         // If this is the server that watched the item get the server user name and skip syncing since it is already in sync
         if (account.serverId == historyItem.serverId)
         {
            embyUserName = account.externalUserName;
            continue;
         }

         if (account.serverType == warp::TracearrServerType::PLEX)
         {
            auto serverName = tracearrApi_->GetServerNameFromId(account.serverId);
            if (!serverName.has_value())
               continue;

            auto* syncPlexApi = apiManager_->GetTracearrPlexApi(serverName.value());
            if (!syncPlexApi)
               continue;

            StateSyncPlexUser::PlexSyncState syncState{
              .item = historyItem,
              .mediaPath = ReplaceMediaPath(itemPath.value(), embyApi->GetMediaPath(), syncPlexApi->GetMediaPath()),
              .api = syncPlexApi,
              .userName = account.externalUserName
            };
            plexUser_->SyncState(syncState, syncServers);
         }
         else if (account.serverType == warp::TracearrServerType::EMBY)
         {
            auto serverName = tracearrApi_->GetServerNameFromId(account.serverId);
            if (!serverName.has_value())
               continue;

            auto* syncEmbyApi = apiManager_->GetTracearrEmbyApi(serverName.value());
            if (!syncEmbyApi)
               continue;

            StateSyncEmbyUser::EmbySyncState syncState{
               .item = historyItem,
               .mediaPath = ReplaceMediaPath(itemPath.value(), embyApi->GetMediaPath(), syncEmbyApi->GetMediaPath()),
               .embyApi = syncEmbyApi,
               .embyUserId = account.externalUserId
            };
            embyUser_->SyncState(syncState, syncServers);
         }
      }

      // If syncServers is not empty, then log the summary of the sync.
      if (!syncServers.empty())
      {
         LogSyncSummary({
            .server = embyApi->GetPrettyName(),
            .user = embyUserName,
            .name = historyItem.fullName,
            .watched = historyItem.watched,
            .playbackPercentage = historyItem.playbackPercentage.has_value() ? historyItem.playbackPercentage.value() : 0,
            .syncResults = syncServers
         });
      }
   }

   void StateSyncUser::Sync(const warp::TracearrHistoryItem& historyItem)
   {
      // Get the tracearr user for this item. If not valid or has only one account skip since nothing to sync with.
      auto tracearrUser = tracearrApi_->GetUser(historyItem.user.id);
      if (!tracearrUser.has_value() || tracearrUser.value().accounts.size() <= 1u)
         return;

      // Sync item with the associated servers and users assigned to this user
      if (historyItem.serverType == warp::TracearrServerType::PLEX)
      {
         SyncPlexState(tracearrUser.value(), historyItem);
      }
      else if (historyItem.serverType == warp::TracearrServerType::EMBY)
      {
         SyncEmbyState(tracearrUser.value(), historyItem);
      }
      else
      {
         //*do-nothing*//
      }
   }
}