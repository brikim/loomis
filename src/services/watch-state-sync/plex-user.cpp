#include "plex-user.h"

#include "logger/log-utils.h"
#include "services/service-utils.h"

namespace loomis
{
   PlexUser::PlexUser(const ServerUser& config,
                      const std::shared_ptr<ApiManager>& apiManager,
                      WatchStateLogger logger)
      : logger_(logger)
      , config_(config)
      , typeServerName_(utils::GetServerName(utils::GetFormattedPlex(), config_.server))
   {
      // Do some quick checking on the users and make sure the api in the config exists.
      // Don't want to check if the user is valid on the api yet since it might be offline.
      // This will be checked every run frame.
      api_ = apiManager->GetPlexApi(config_.server);
      trackerApi_ = apiManager->GetTautulliApi(config_.server);
      if (api_ && trackerApi_)
      {
         // Will get users from tautulli for plex. Do a small pre-check and warn the system.
         if (trackerApi_->GetValid() && !trackerApi_->GetUserInfo(config_.user_name))
         {
            logger_.LogWarning("{} not found on {}. Is user name correct?",
                               utils::GetTag("user", config_.user_name),
                               utils::GetServerName(utils::GetFormattedTautulli(), config_.server));
         }

         valid_ = true;
      }
      else
      {
         if (!api_)
         {
            logger_.LogWarning("{} api not found for {}",
                               typeServerName_,
                               utils::GetTag("user", config_.user_name));
         }

         if (!trackerApi_)
         {
            logger_.LogWarning("{} tracker api not found for {}. Required for this service.",
                               utils::GetServerName(utils::GetFormattedTautulli(), config_.server),
                               utils::GetTag("user", config_.user_name));
         }
      }
   }

   bool PlexUser::GetValid() const
   {
      return valid_;
   }

   std::string PlexUser::GetServerAndUserName() const
   {
      return typeServerName_ + ":" + config_.user_name;
   }

   int32_t PlexUser::GetId() const
   {
      return userInfo_.id;
   }

   std::string_view PlexUser::GetServerName() const
   {
      return config_.server;
   }

   std::string_view PlexUser::GetTypeAndServerName() const
   {
      return typeServerName_;
   }

   std::string_view PlexUser::GetUser() const
   {
      return userInfo_.friendlyName.empty() ? config_.user_name : userInfo_.friendlyName;
   }

   std::optional<TautulliHistoryItems> PlexUser::GetWatchHistory(std::string_view historyDate)
   {
      return trackerApi_->GetWatchHistoryForUser(config_.user_name, historyDate);
   }

   void PlexUser::Update()
   {
      auto userInfo{trackerApi_->GetUserInfo(config_.user_name)};
      valid_ = userInfo.has_value();
      if (valid_) userInfo_ = *userInfo;
   }

   void PlexUser::SyncStateWithPlex()
   {
      // Currently not supported. Future Growth?
   }

   bool PlexUser::SyncEmbyWatchedState(const EmbySyncState& syncState)
   {
      auto info = api_->GetItemInfo(syncState.name);
      if (!info) return false;

      auto iter = std::ranges::find_if(info->items, [&](const auto& item) {
         return item.path == ReplaceMediaPath(syncState.path, syncState.mediaPath, api_->GetMediaPath());
      });

      // If the item was not found or is already watched just return
      if (iter == info->items.end() || iter->watched) return false;

      return api_->SetWatched(iter->ratingKey);
   }

   bool PlexUser::SyncEmbyPlayState(const EmbySyncState& syncState)
   {
      auto info = api_->GetItemInfo(syncState.name);
      if (!info) return false;

      // Find the correct item based on the path
      auto iter = std::ranges::find_if(info->items, [&](const auto& item) {
         return item.path == ReplaceMediaPath(syncState.path, syncState.mediaPath, api_->GetMediaPath());
      });

      // If the item was not found or is already watched just return
      if (iter == info->items.end() || iter->playbackPercentage == syncState.playbackPercentage) return false;

      auto msLocation = iter->durationMs * static_cast<int64_t>(syncState.playbackPercentage) / 100;
      return api_->SetPlayed(iter->ratingKey, msLocation);
   }

   void PlexUser::SyncStateWithEmby(const EmbySyncState& syncState, std::string& syncResults)
   {
      if (!config_.can_sync) return;

      if (syncState.watched ? SyncEmbyWatchedState(syncState) : SyncEmbyPlayState(syncState))
      {
         syncResults = utils::BuildSyncServerString(syncResults, utils::GetFormattedPlex(), config_.server);
      }
   }
}