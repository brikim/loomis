#include "plex-user.h"

#include "services/service-utils.h"

#include <warp/log/log-utils.h>
#include <warp/utils.h>

namespace loomis
{
   StateSyncPlexUser::StateSyncPlexUser(const ServerPlexUser& config,
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
      api_ = apiManager->GetPlexApi(config_.server);
      trackerApi_ = apiManager->GetTautulliApi(config_.server);
      if (api_ && trackerApi_)
      {
         // Will get users from tautulli for plex. Do a small pre-check and warn the system.
         if (trackerApi_->GetValid() && !trackerApi_->GetUserInfo(config_.userName))
         {
            logger_.LogWarning("{} not found on {}. Is user name correct?",
                               warp::GetTag("user", config_.userName),
                               trackerApi_->GetPrettyName());
         }

         valid_ = true;
      }
      else
      {
         if (!api_)
         {
            logger_.LogWarning("{} api not found for {}",
                               warp::GetServerName(warp::GetFormattedPlex(), config_.server),
                               warp::GetTag("user", config_.userName));
         }

         if (!trackerApi_)
         {
            logger_.LogWarning("{} tracker api not found for {}. Required for this service.",
                               warp::GetServerName(warp::GetFormattedTautulli(), config_.server),
                               warp::GetTag("user", config_.userName));
         }
      }
   }

   bool StateSyncPlexUser::GetValid() const
   {
      return valid_;
   }

   std::string StateSyncPlexUser::GetServerAndUserName() const
   {
      return api_->GetPrettyName() + ":" + config_.userName;
   }

   int32_t StateSyncPlexUser::GetId() const
   {
      return userInfo_.id;
   }

   std::string_view StateSyncPlexUser::GetServerName() const
   {
      return config_.server;
   }

   std::optional<std::string> StateSyncPlexUser::GetTracearrServerName() const
   {
      return api_->GetTracearrServerName();
   }

   std::string_view StateSyncPlexUser::GetTypeAndServerName() const
   {
      return api_->GetPrettyName();
   }

   std::string_view StateSyncPlexUser::GetUser() const
   {
      return userInfo_.friendlyName.empty() ? config_.userName : userInfo_.friendlyName;
   }

   std::optional<warp::TautulliHistoryItems> StateSyncPlexUser::GetWatchHistory(std::string_view historyDate, int64_t epochHistoryTime)
   {
      return trackerApi_->GetWatchHistoryForUser(config_.userName, historyDate, epochHistoryTime);
   }

   void StateSyncPlexUser::Update()
   {
      auto userInfo = trackerApi_->GetUserInfo(config_.userName);
      valid_ = userInfo.has_value();
      if (valid_)
         userInfo_ = *userInfo;
   }

   void StateSyncPlexUser::SyncStateWithPlex()
   {
      // Currently not supported. Future Growth?
   }

   std::optional<warp::PlexSearchResult> StateSyncPlexUser::GetSyncStateItem(const EmbySyncState& syncState) const
   {
      std::optional<warp::PlexSearchResults> info;
      if (config_.token)
         info = api_->GetItemInfoByPathWithToken(*config_.token, syncState.path);
      else
         info = api_->GetItemInfoByPathWithUserName(config_.userName, syncState.path);

      if (!info || info->items.empty())
         return std::nullopt;

      const auto targetPath = ReplaceMediaPath(
          syncState.path,
          syncState.mediaPath,
          api_->GetMediaPath()
      );

      auto it = std::ranges::find_if(info->items, [&](const auto& item) {
         return std::ranges::any_of(item.paths, [&](const auto& path) {
            return path == targetPath;
         });
      });

      // If no matching item is found, we are done.
      if (it == info->items.end())
         return std::nullopt;

      return *it;
   }

   bool StateSyncPlexUser::SyncEmbyWatchedState(const EmbySyncState& syncState)
   {
      auto item = GetSyncStateItem(syncState);
      if (!item || item->watched)
         return false;

      if (!dryRun_)
      {
         if (config_.token)
            return api_->SetWatchedByUserToken(*config_.token, item->ratingKey);
         else
            return api_->SetWatchedByUserName(config_.userName, item->ratingKey);
      }
      else
         return true;
   }

   bool StateSyncPlexUser::SyncEmbyPlayState(const EmbySyncState& syncState)
   {
      auto item = GetSyncStateItem(syncState);
      if (!item || item->playbackPercentage == syncState.item->playbackPercentage)
         return false;

      auto msLocation = (item->durationMs * static_cast<int64_t>(syncState.item->playbackPercentage)) / 100;

      if (!dryRun_)
      {
         if (config_.token)
            return api_->SetPlayedByUserToken(*config_.token, item->ratingKey, msLocation);
         else
            return api_->SetPlayedByUserName(config_.userName, item->ratingKey, msLocation);
      }
      else
         return true;
   }

   void StateSyncPlexUser::SyncStateWithEmby(const EmbySyncState& syncState, std::string& syncResults)
   {
      if (!config_.token && !api_->GetUserTokenAvailable(config_.userName))
         return;


      if (syncState.item->watched ? SyncEmbyWatchedState(syncState) : SyncEmbyPlayState(syncState))
      {
         syncResults = warp::BuildSyncServerString(syncResults, warp::GetFormattedPlex(), config_.server);
      }
   }
}