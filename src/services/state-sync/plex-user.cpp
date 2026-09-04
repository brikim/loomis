#include "plex-user.h"

#include "services/service-utils.h"

#include <warp/log/log-utils.h>
#include <warp/utils.h>

namespace loomis
{
   StateSyncPlexUser::StateSyncPlexUser(bool dryRun,
                                        ServiceLogger logger)
      : dryRun_(dryRun)
      , logger_(logger)
   {}

   std::optional<warp::PlexSearchResult> StateSyncPlexUser::GetSyncStateItem(const PlexSyncState& syncState) const
   {
      auto info = syncState.api->GetItemInfoByPathWithUserName(syncState.userName, syncState.mediaPath);
      if (!info || info->items.empty())
         return std::nullopt;

      auto it = std::ranges::find_if(info->items, [&syncState](const auto& item) {
         return std::ranges::any_of(item.paths, [&syncState](const auto& path) {
            return path == syncState.mediaPath;
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
         return syncState.api->SetWatchedByUserName(syncState.userName, item->ratingKey);
      else
         return true;
   }

   bool StateSyncPlexUser::SyncPlayState(const PlexSyncState& syncState)
   {
      auto item = GetSyncStateItem(syncState);
      if (!item || !syncState.item.playbackPercentage.has_value() || item->playbackPercentage == syncState.item.playbackPercentage.value())
         return false;

      auto msLocation = (item->durationMs * static_cast<int64_t>(syncState.item.playbackPercentage.value())) / 100;

      if (!dryRun_)
         return syncState.api->SetPlayedByUserName(syncState.userName, item->ratingKey, msLocation);
      else
         return true;
   }

   void StateSyncPlexUser::SyncState(const PlexSyncState& syncState, std::string& syncResults)
   {
      if (!syncState.api->GetUserTokenAvailable(syncState.userName))
         return;


      if (syncState.item.watched ? SyncWatchedState(syncState) : SyncPlayState(syncState))
      {
         syncResults = warp::BuildSyncServerString(syncResults, warp::GetFormattedPlex(), syncState.api->GetName());
      }
   }
}