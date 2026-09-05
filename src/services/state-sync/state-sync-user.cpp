#include "state-sync-user.h"

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
      , plexUser_(dryRun, logger_)
      , embyUser_(dryRun, logger_)
   {}

   void StateSyncUser::LogSyncSummary(const LogSyncData& syncSummary)
   {
      if (syncSummary.watched)
      {
         logger_.LogInfo("{}{} watched {} on {} sync {} watch state",
                         dryRunText_,
                         warp::GetStandoutText(syncSummary.user),
                         warp::GetStandoutText(syncSummary.name),
                         syncSummary.server,
                         syncSummary.syncResults);
      }
      else
      {
         logger_.LogInfo("{}{} played {}% of {} on {} sync {} play state",
                         dryRunText_,
                         warp::GetStandoutText(syncSummary.user),
                         syncSummary.playbackPercentage,
                         warp::GetStandoutText(syncSummary.name),
                         syncSummary.server,
                         syncSummary.syncResults);
      }
   }

   void StateSyncUser::SyncPlexState(const warp::TracearrUser& user, const warp::TracearrHistoryItem& historyItem)
   {
      SyncStateInternal(user, historyItem, apiManager_->GetTracearrPlexApi(historyItem.serverName));
   }

   void StateSyncUser::SyncEmbyState(const warp::TracearrUser& user, const warp::TracearrHistoryItem& historyItem)
   {
      SyncStateInternal(user, historyItem, apiManager_->GetTracearrEmbyApi(historyItem.serverName));
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