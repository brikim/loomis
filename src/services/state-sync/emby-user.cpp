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
      api_ = apiManager->GetEmbyApi(config_.server);
      if (api_ && api_->GetTracearrServerName())
      {
         // Will get users from emby. Do a small pre-check and warn the system.
         if (api_->GetValid() && !api_->GetUser(config_.userName))
         {
            logger_.LogWarning("{} not found on {}. Is user name correct?",
                               warp::GetTag("user", config_.userName),
                               api_->GetPrettyName());
         }

         valid_ = true;
      }
      else
      {
         if (api_ && !api_->GetTracearrServerName())
            logger_.LogWarning("{} api does not contain a Tracearr server name. Required for service.",
                                  warp::GetServerName(warp::GetFormattedEmby(), config_.server));

         if (!api_)
            logger_.LogWarning("{} api not found for {}",
                               warp::GetServerName(warp::GetFormattedEmby(), config.server),
                               warp::GetTag("user", config_.userName));
      }
   }

   bool StateSyncEmbyUser::GetValid() const
   {
      return valid_;
   }

   std::string StateSyncEmbyUser::GetServerAndUserName() const
   {
      return api_->GetPrettyName() + ":" + config_.userName;
   }

   std::string_view StateSyncEmbyUser::GetServerName() const
   {
      return config_.server;
   }

   std::optional<std::string> StateSyncEmbyUser::GetTracearrServerName() const
   {
      return api_->GetTracearrServerName();
   }

   std::string_view StateSyncEmbyUser::GetTypeAndServerName() const
   {
      return api_->GetPrettyName();
   }

   std::string_view StateSyncEmbyUser::GetUser() const
   {
      return config_.userName;
   }

   const std::filesystem::path& StateSyncEmbyUser::GetMediaPath() const
   {
      return api_->GetMediaPath();
   }

   std::optional<warp::EmbyPlayState> StateSyncEmbyUser::GetPlayState(std::string_view id)
   {
      return api_->GetPlayState(userId_, id);
   }

   void StateSyncEmbyUser::Update()
   {
      auto user = api_->GetUser(config_.userName);
      valid_ = user.has_value();
      if (valid_)
         userId_ = std::move(user->id);
   }

   bool StateSyncEmbyUser::SyncWatchedState(std::string_view id)
   {
      if (api_->GetWatchedStatus(userId_, id))
         return false;

      if (!dryRun_)
         return api_->SetWatchedStatus(userId_, id);
      else
         return true;
   }

   bool StateSyncEmbyUser::SyncPlayState(const EmbySyncState& syncState, std::string_view id)
   {
      auto playState = api_->GetPlayState(userId_, id);
      if (!playState || syncState.item->playbackPercentage == std::lround(playState->percentage))
         return false;

      if (!dryRun_)
      {
         int64_t tickLocation = std::llround(static_cast<double>(playState->runTimeTicks) * (static_cast<double>(syncState.item->playbackPercentage) / 100.0));
         return api_->SetPlayState(userId_, id, tickLocation, syncState.item->watchTime);
      }
      else
         return true;
   }

   void StateSyncEmbyUser::SyncState(const EmbySyncState& syncState, std::string& syncResults)
   {
      auto id = api_->GetIdFromPath(ReplaceMediaPath(syncState.path, syncState.mediaPath, GetMediaPath()));
      if (!id)
         return;

      bool forceWatched = syncState.item->watched || syncState.item->playbackPercentage >= playbackPercentageThreshold;
      bool success = forceWatched ? SyncWatchedState(*id) : SyncPlayState(syncState, *id);
      if (success)
      {
         syncResults = warp::BuildSyncServerString(syncResults, warp::GetFormattedEmby(), config_.server);
      }
   }
}