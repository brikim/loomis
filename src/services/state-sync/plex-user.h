#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-plex.h>
#include <warp/types.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace loomis
{
   class StateSyncPlexUser
   {
   public:
      StateSyncPlexUser(bool dryRun,
                        ServiceLogger logger);
      ~StateSyncPlexUser() = default;

      struct PlexSyncState
      {
         const warp::TracearrHistoryItem& item;
         const std::filesystem::path& mediaPath;
         warp::PlexApi* api{nullptr};
         std::string_view userName;
      };
      void SyncState(const PlexSyncState& syncState, std::string& syncResults);

   private:
      std::optional<warp::PlexSearchResult> GetSyncStateItem(const PlexSyncState& syncState) const;
      bool SyncWatchedState(const PlexSyncState& syncState);
      bool SyncPlayState(const PlexSyncState& syncState);

      bool dryRun_{false};
      ServiceLogger logger_;
   };
}