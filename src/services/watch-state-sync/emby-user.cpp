#include "emby-user.h"

#include "logger/log-utils.h"

namespace loomis
{
   EmbyUser::EmbyUser(const ServerUser& config,
                      ApiManager* apiManager,
                      WatchStateLogger logger)
      : logger_(logger)
      , config_(config)
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

   void EmbyUser::Update()
   {
      auto user = embyApi_->GetUser(config_.user_name);
      valid_ = user.has_value();
      if (valid_) userId_ = std::move(user->id);
   }

   bool EmbyUser::SyncWatchedState(const TautulliHistoryItem* item, const std::string& path)
   {
      auto id = embyApi_->GetIdFromPathMap(path);
      if (!id) return false;

      // If this item is already watched just return
      if (embyApi_->GetWatchedStatus(userId_, *id)) return false;

      embyApi_->SetWatchedStatus(userId_, *id);

      return true;
   }

   bool EmbyUser::SyncPlayState(const TautulliHistoryItem* item, const std::string& path)
   {
      auto id = embyApi_->GetIdFromPathMap(path);
      if (!id) return false;

      auto playState = embyApi_->GetPlayState(userId_, *id);
      if (!playState || item->playbackPercentage == std::lround(playState->percentage)) return false;

      int64_t tickLocation = std::llround(static_cast<double>(playState->runTimeTicks) * (static_cast<double>(item->playbackPercentage) / 100.0));

      auto tp = std::chrono::sys_time<std::chrono::seconds>{std::chrono::seconds{item->timeWatchedEpoch}};
      auto timeString = std::format("{:%FT%TZ}", tp);

      return embyApi_->SetPlayState(userId_, *id, tickLocation, timeString);
   }

   void EmbyUser::SyncStateWithPlex(const TautulliHistoryItem* item, const std::string& path, std::string& target)
   {
      if (item->watched ? SyncWatchedState(item, path) : SyncPlayState(item, path))
      {
         target = utils::BuildTargetString(target, utils::GetFormattedEmby(), config_.server);
      }
   }
}