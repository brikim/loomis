#include "emby-user.h"

#include "services/service-utils.h"

#include <warp/log/log-utils.h>
#include <warp/utils.h>

namespace loomis
{
   namespace
   {
      constexpr int32_t playbackPercentageThreshold{99};
   }

   StateSyncEmbyUser::StateSyncEmbyUser(const ServerUser& config,
                                        bool dryRun,
                                        const std::shared_ptr<warp::ApiManager>& apiManager,
                                        ServiceLogger logger)
      : logger_(logger)
      , dryRun_(dryRun)
      , config_(config)
   {
      // Do some quick checking on the users and make sure the api in the config exists.
      // Don't want to check if the user is valid on the api yet since it might be offline.
      // This will be checked every run frame.
      embyApi_ = apiManager->GetEmbyApi(config_.server);
      if (embyApi_)
      {
         // Will get users from emby. Do a small pre-check and warn the system.
         if (embyApi_->GetValid() && !embyApi_->GetUser(config_.userName))
         {
            logger_.LogWarning("{} not found on {}. Is user name correct?",
                               warp::GetTag("user", config_.userName),
                               embyApi_->GetPrettyName());
         }

         valid_ = true;
      }
      else
      {
         if (!embyApi_)
         {
            logger_.LogWarning("{} api not found for {}",
                               warp::GetServerName(warp::GetFormattedEmby(), config.server),
                               warp::GetTag("user", config_.userName));
         }
      }
   }

   bool StateSyncEmbyUser::GetValid() const
   {
      return valid_;
   }

   std::string StateSyncEmbyUser::GetServerAndUserName() const
   {
      return embyApi_->GetPrettyName() + ":" + config_.userName;
   }

   std::string_view StateSyncEmbyUser::GetServerName() const
   {
      return config_.server;
   }

   std::optional<std::string> StateSyncEmbyUser::GetTracearrServerName() const
   {
      return embyApi_->GetTracearrServerName();
   }

   std::string_view StateSyncEmbyUser::GetTypeAndServerName() const
   {
      return embyApi_->GetPrettyName();
   }

   std::string_view StateSyncEmbyUser::GetUser() const
   {
      return config_.userName;
   }

   const std::filesystem::path& StateSyncEmbyUser::GetMediaPath() const
   {
      return embyApi_->GetMediaPath();
   }

   std::optional<warp::EmbyPlayState> StateSyncEmbyUser::GetPlayState(std::string_view id)
   {
      return embyApi_->GetPlayState(userId_, id);
   }

   void StateSyncEmbyUser::Update()
   {
      auto user = embyApi_->GetUser(config_.userName);
      valid_ = user.has_value();
      if (valid_)
         userId_ = std::move(user->id);
   }

   bool StateSyncEmbyUser::SyncPlexWatchedState(std::string_view embyId, const warp::TracearrHistoryItem* historyItem)
   {
      // If this item is already watched just return
      if (embyApi_->GetWatchedStatus(userId_, embyId))
         return false;

      if (!dryRun_)
         embyApi_->SetWatchedStatus(userId_, embyId);

      return true;
   }

   bool StateSyncEmbyUser::SyncPlexPlayState(std::string_view embyId, const warp::TracearrHistoryItem* historyItem)
   {
      auto playState = embyApi_->GetPlayState(userId_, embyId);
      if (!playState || historyItem->playbackPercentage == std::lround(playState->percentage))
         return false;

      int64_t tickLocation = std::llround(static_cast<double>(playState->runTimeTicks) * (static_cast<double>(historyItem->playbackPercentage) / 100.0));
      if (tickLocation == playState->runTimeTicks)
      {
         return SyncPlexWatchedState(embyId, historyItem);
      }

      if (!dryRun_)
         return embyApi_->SetPlayState(userId_, embyId, tickLocation, historyItem->watchTime);
      else
         return true;
   }

   void StateSyncEmbyUser::SyncStateWithPlex(const warp::TracearrHistoryItem* item,
                                             const std::filesystem::path& itemPath,
                                             std::string& syncResults)
   {
      auto id = embyApi_->GetIdFromPath(itemPath);
      if (!id)
         return;

      bool forceWatched = item->watched || item->playbackPercentage >= playbackPercentageThreshold;
      bool success = forceWatched ? SyncPlexWatchedState(id.value(), item) : SyncPlexPlayState(id.value(), item);
      if (success)
      {
         syncResults = warp::BuildSyncServerString(syncResults, warp::GetFormattedEmby(), config_.server);
      }
   }

   bool StateSyncEmbyUser::SyncEmbyWatchedState(std::string_view id)
   {
      if (embyApi_->GetWatchedStatus(userId_, id))
         return false;

      if (!dryRun_)
         return embyApi_->SetWatchedStatus(userId_, id);
      else
         return true;
   }

   bool StateSyncEmbyUser::SyncEmbyPlayState(const EmbySyncState& syncState, std::string_view id)
   {
      auto playState = embyApi_->GetPlayState(userId_, id);
      if (!playState || syncState.item->playbackPercentage == std::lround(playState->percentage))
         return false;

      if (!dryRun_)
      {
         int64_t tickLocation = std::llround(static_cast<double>(playState->runTimeTicks) * (static_cast<double>(syncState.item->playbackPercentage) / 100.0));
         return embyApi_->SetPlayState(userId_, id, tickLocation, syncState.item->watchTime);
      }
      else
         return true;
   }

   void StateSyncEmbyUser::SyncStateWithEmby(const EmbySyncState& syncState, std::string& syncResults)
   {
      auto id = embyApi_->GetIdFromPath(ReplaceMediaPath(syncState.path, syncState.mediaPath, GetMediaPath()));
      if (!id)
         return;

      bool forceWatched = syncState.item->watched || syncState.item->playbackPercentage >= playbackPercentageThreshold;
      bool success = forceWatched ? SyncEmbyWatchedState(*id) : SyncEmbyPlayState(syncState, *id);
      if (success)
      {
         syncResults = warp::BuildSyncServerString(syncResults, warp::GetFormattedEmby(), config_.server);
      }
   }
}