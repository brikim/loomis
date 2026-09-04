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

   StateSyncEmbyUser::StateSyncEmbyUser(bool dryRun,
                                        ServiceLogger logger)
      : logger_(logger)
      , dryRun_(dryRun)
   {}

   bool StateSyncEmbyUser::SyncWatchedState(EmbySyncState& syncState, std::string_view id)
   {
      if (syncState.embyApi->GetWatchedStatus(syncState.embyUserId, id))
         return false;

      if (!dryRun_)
         return syncState.embyApi->SetWatchedStatus(syncState.embyUserId, id);
      else
         return true;
   }

   bool StateSyncEmbyUser::SyncPlayState(EmbySyncState& syncState, std::string_view id)
   {
      auto playState = syncState.embyApi->GetPlayState(syncState.embyUserId, id);
      if (!playState || !syncState.item.playbackPercentage.has_value() || syncState.item.playbackPercentage.value() == std::lround(playState->percentage))
         return false;

      if (!dryRun_)
      {
         int64_t tickLocation = std::llround(static_cast<double>(playState->runTimeTicks) * (static_cast<double>(syncState.item.playbackPercentage.value()) / 100.0));
         return syncState.embyApi->SetPlayState(syncState.embyUserId, id, tickLocation, syncState.item.watchTime);
      }
      else
         return true;
   }

   void StateSyncEmbyUser::SyncState(EmbySyncState syncState, std::string& syncResults)
   {
      auto id = syncState.embyApi->GetIdFromPath(syncState.mediaPath);
      if (!id)
         return;

      bool forceWatched = syncState.item.watched || syncState.item.playbackPercentage >= playbackPercentageThreshold;
      bool success = forceWatched ? SyncWatchedState(syncState, *id) : SyncPlayState(syncState, *id);
      if (success)
      {
         syncResults = warp::BuildSyncServerString(syncResults, warp::GetFormattedEmby(), syncState.embyApi->GetName());
      }
   }
}