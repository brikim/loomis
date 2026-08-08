#include "plex-user.h"

#include "services/service-utils.h"

#include <warp/log/log-utils.h>
#include <warp/utils.h>

namespace loomis
{
   StateSyncPlexUser::StateSyncPlexUser(const ServerPlexUser& config,
                                        std::string_view tracearrUserName,
                                        bool dryRun,
                                        const std::shared_ptr<warp::ApiManager>& apiManager,
                                        ServiceLogger logger)
      : dryRun_(dryRun)
      , tracearrUserName_(tracearrUserName)
      , logger_(logger)
      , config_(config)
   {
      api_ = apiManager->GetPlexApi(config_.server);
      if (api_ && api_->GetTracearrServerName())
         valid_ = true;
      else
      {
         if (api_ && !api_->GetTracearrServerName())
            logger_.LogWarning("{} api does not contain a Tracearr server name. Required for service.",
                                  warp::GetServerName(warp::GetFormattedPlex(), config_.server));

         if (!api_)
            logger_.LogWarning("{} api not found for {}",
                               warp::GetServerName(warp::GetFormattedPlex(), config_.server),
                               warp::GetTag("user", config_.userName));
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
      return tracearrUserName_;
   }

   const std::filesystem::path& StateSyncPlexUser::GetMediaPath() const
   {
      return api_->GetMediaPath();
   }

   std::optional<warp::PlexSearchResult> StateSyncPlexUser::GetSyncStateItem(const PlexSyncState& syncState) const
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

   bool StateSyncPlexUser::SyncWatchedState(const PlexSyncState& syncState)
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

   bool StateSyncPlexUser::SyncPlayState(const PlexSyncState& syncState)
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

   void StateSyncPlexUser::SyncState(const PlexSyncState& syncState, std::string& syncResults)
   {
      if (!config_.token && !api_->GetUserTokenAvailable(config_.userName))
         return;


      if (syncState.item->watched ? SyncWatchedState(syncState) : SyncPlayState(syncState))
      {
         syncResults = warp::BuildSyncServerString(syncResults, warp::GetFormattedPlex(), config_.server);
      }
   }
}