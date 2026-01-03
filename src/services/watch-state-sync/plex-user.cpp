#include "plex-user.h"

#include "logger/log-utils.h"

namespace loomis
{
   PlexUser::PlexUser(const ServerUser& config,
                      ApiManager* apiManager,
                      WatchStateLogger logger)
      : logger_(logger)
      , config_(config)
   {
      // Do some quick checking on the users and make sure the api in the config exists.
      // Don't want to check if the user is valid on the api yet since it might be offline.
      // This will be checked every run frame.
      api_ = apiManager->GetPlexApi(config_.server);
      trackerApi_ = apiManager->GetTautulliApi(config_.server);
      if (api_ && trackerApi_)
      {
         // Will get users from tautulli for plex. Do a small pre-check and warn the system.
         if (trackerApi_->GetValid() && !trackerApi_->GetUserInfo(config_.user).has_value())
         {
            logger_.LogWarning("{} not found on {}. Is user name correct?",
                               utils::GetTag("user", config_.user),
                               utils::GetServerName(utils::GetFormattedTautulli(), config_.server));
         }

         valid_ = true;
      }
      else
      {
         if (!api_)
         {
            logger_.LogWarning("{} api not found for {}",
                               utils::GetServerName(utils::GetFormattedPlex(), config_.server),
                               utils::GetTag("user", config_.user));
         }

         if (!trackerApi_)
         {
            logger_.LogWarning("{} tracker api not found for {}. Required for this service.",
                               utils::GetServerName(utils::GetFormattedTautulli(), config_.server),
                               utils::GetTag("user", config_.user));
         }
      }
   }

   bool PlexUser::GetValid() const
   {
      return valid_;
   }

   int32_t PlexUser::GetId() const
   {
      return userInfo_.id;
   }

   std::string_view PlexUser::GetServer() const
   {
      return config_.server;
   }

   std::optional<std::string_view> PlexUser::GetFriendlyName() const
   {
      if (userInfo_.friendlyNameValid)
      {
         return userInfo_.friendlyName;
      }
      return std::nullopt;
   }

   std::optional<TautulliHistoryItems> PlexUser::GetWatchHistory(std::string_view historyDate)
   {
      return trackerApi_->GetWatchHistoryForUser(config_.user, historyDate);
   }

   void PlexUser::Update()
   {
      auto userInfo{trackerApi_->GetUserInfo(config_.user)};
      valid_ = userInfo.has_value();
      if (valid_) userInfo_ = *userInfo;
   }

   void PlexUser::SyncStateWithPlex(const TautulliHistoryItem* item, std::string_view path, std::string& target)
   {
      // Currently not supported. Future Growth?
   }
}