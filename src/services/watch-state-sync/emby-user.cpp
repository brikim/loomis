#include "emby-user.h"

#include "logger/log-utils.h"
#include "services/service-utils.h"

namespace loomis
{
   EmbyUser::EmbyUser(const ServerUser& config,
                      const std::shared_ptr<ApiManager>& apiManager,
                      WatchStateLogger logger)
      : logger_(logger)
      , config_(config)
      , serverName_(config_.server)
      , prettyServerName_(utils::GetServerName(utils::GetFormattedEmby(), config_.server))
   {
      // Do some quick checking on the users and make sure the api in the config exists.
      // Don't want to check if the user is valid on the api yet since it might be offline.
      // This will be checked every run frame.
      embyApi_ = apiManager->GetEmbyApi(config_.server);
      jellystatApi_ = apiManager->GetJellystatApi(config_.server);
      if (embyApi_ && jellystatApi_)
      {
         // Will get users from emby. Do a small pre-check and warn the system.
         if (embyApi_->GetValid() && !embyApi_->GetUser(config_.user_name))
         {
            logger_.LogWarning("{} not found on {}. Is user name correct?",
                               utils::GetTag("user", config_.user_name),
                               utils::GetServerName(utils::GetFormattedEmby(), config_.server));
         }

         valid_ = true;
      }
      else
      {
         if (!embyApi_)
         {
            logger_.LogWarning("{} api not found for {}",
                               utils::GetServerName(utils::GetFormattedEmby(), config_.server),
                               utils::GetTag("user", config_.user_name));
         }

         if (!jellystatApi_)
         {
            logger_.LogWarning("{} tracker api not found for {}. Required for this service.",
                               utils::GetServerName(utils::GetFormattedJellystat(), config_.server),
                               utils::GetTag("user", config_.user_name));
         }
      }
   }

   bool EmbyUser::GetValid() const
   {
      return valid_;
   }

   std::string_view EmbyUser::GetServerName() const
   {
      return serverName_;
   }

   std::string_view EmbyUser::GetPrettyServerName() const
   {
      return prettyServerName_;
   }

   const std::string& EmbyUser::GetMediaPath() const
   {
      return embyApi_->GetMediaPath();
   }

   std::optional<EmbyPlayState> EmbyUser::GetPlayState(std::string_view id)
   {
      return embyApi_->GetPlayState(userId_, id);
   }

   void EmbyUser::Update()
   {
      auto user = embyApi_->GetUser(config_.user_name);
      valid_ = user.has_value();
      if (valid_) userId_ = std::move(user->id);
   }

   std::optional<JellystatHistoryItems> EmbyUser::GetWatchHistory()
   {
      return jellystatApi_->GetWatchHistoryForUser(userId_);
   }

   bool EmbyUser::SyncPlexWatchedState(const std::string& plexPath)
   {
      auto id = embyApi_->GetIdFromPathMap(plexPath);
      if (!id) return false;

      // If this item is already watched just return
      if (embyApi_->GetWatchedStatus(userId_, *id)) return false;

      embyApi_->SetWatchedStatus(userId_, *id);

      return true;
   }

   bool EmbyUser::SyncPlexPlayState(const PlexSyncState& syncState)
   {
      auto id = embyApi_->GetIdFromPathMap(syncState.path);
      if (!id) return false;

      auto playState = embyApi_->GetPlayState(userId_, *id);
      if (!playState || syncState.playbackPercentage == std::lround(playState->percentage)) return false;

      int64_t tickLocation = std::llround(static_cast<double>(playState->runTimeTicks) * (static_cast<double>(syncState.playbackPercentage) / 100.0));

      auto timeString = GetIsoTimeStr(std::chrono::sys_time<std::chrono::seconds>{std::chrono::seconds{syncState.timeWatchedEpoch}});
      return embyApi_->SetPlayState(userId_, *id, tickLocation, timeString);
   }

   void EmbyUser::SyncStateWithPlex(const PlexSyncState& syncState, std::string& syncResults)
   {
      if (syncState.watched ? SyncPlexWatchedState(syncState.path) : SyncPlexPlayState(syncState))
      {
         syncResults = utils::BuildSyncServerString(syncResults, utils::GetFormattedEmby(), config_.server);
      }
   }

   bool EmbyUser::SyncEmbyWatchedState(std::string_view id)
   {
      if (embyApi_->GetWatchedStatus(userId_, id)) return false;
      return embyApi_->SetWatchedStatus(userId_, id);
   }

   bool EmbyUser::SyncEmbyPlayState(const EmbySyncState& syncState, std::string_view id)
   {
      auto playState = embyApi_->GetPlayState(userId_, id);
      if (!playState || syncState.playbackPercentage == std::lround(playState->percentage)) return false;

      int64_t tickLocation = std::llround(static_cast<double>(playState->runTimeTicks) * (static_cast<double>(syncState.playbackPercentage) / 100.0));
      return embyApi_->SetPlayState(userId_, id, tickLocation, syncState.timeWatched);
   }

   void EmbyUser::SyncStateWithEmby(const EmbySyncState& syncState, std::string& syncResults)
   {
      auto id = embyApi_->GetIdFromPathMap(ReplaceMediaPath(syncState.path, syncState.mediaPath, GetMediaPath()));
      if (!id) return;

      if (syncState.watched ? SyncEmbyWatchedState(*id) : SyncEmbyPlayState(syncState, *id))
      {
         syncResults = utils::BuildSyncServerString(syncResults, utils::GetFormattedEmby(), config_.server);
      }
   }
}