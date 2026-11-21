#include "state-sync-user.h"

#include "services/service-utils.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-plex.h>
#include <warp/api/api-tracearr.h>
#include <warp/log/log-utils.h>

namespace loomis
{
   StateSyncUser::StateSyncUser(bool dryRun,
                                std::shared_ptr<warp::ApiManager> apiManager,
                                ServiceLogger logger)
      : apiManager_(std::move(apiManager))
      , logger_(logger)
      , dryRunText_(GetDryRunText(dryRun))
      , tracearrApi_(apiManager_->GetTracearrApi())
      , plexUser_(dryRun, logger_)
      , embyUser_(dryRun, logger_)
   {
   }

   void StateSyncUser::LogSyncSummary(const LogSyncData& syncSummary)
   {
      if (syncSummary.watched)
      {
         logger_.LogInfo(
            "{}{} watched {} on {} sync {} watch state",
            dryRunText_,
            warp::GetStandoutText(syncSummary.user),
            warp::GetStandoutText(syncSummary.name),
            syncSummary.server,
            syncSummary.syncResults);
      }
      else
      {
         logger_.LogInfo(
            "{}{} played {}% of {} on {} sync {} play state",
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

   void StateSyncUser::SyncHistoryItem(const warp::TracearrUser& user,
                                       const warp::TracearrHistoryItem& historyItem)
   {
      if (historyItem.serverType == warp::TracearrServerType::PLEX)
      {
         SyncPlexState(user, historyItem);
      }
      else if (historyItem.serverType == warp::TracearrServerType::EMBY)
      {
         SyncEmbyState(user, historyItem);
      }
      else
      {
         //*do-nothing*//
      }
   }

   void StateSyncUser::Sync(std::string_view userId,
                            const std::vector<const warp::TracearrHistoryItem*>& historyItems)
   {
      if (historyItems.empty())
         return;

      // Get the Tracearr user once for all history items belonging to this user.
      auto tracearrUser = tracearrApi_->GetUserById(userId);
      if (!tracearrUser.has_value() || (*tracearrUser).accounts.size() <= 1u)
         return;

      for (const auto* historyItem : historyItems)
      {
         try
         {
            SyncHistoryItem(*tracearrUser, *historyItem);
         }
         catch (const std::exception& e)
         {
            logger_.LogWarning(
               "Encountered a problem for {} {} sync: {}",
               warp::GetTag("user", historyItem->user.name),
               warp::GetTag("name", historyItem->fullName),
               warp::GetTag("error", e.what()));
         }
      }
   }
} // namespace loomis
