#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"
#include "services/service-utils.h"
#include "services/state-sync/emby-user.h"
#include "services/state-sync/plex-user.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-tracearr-types.h>
#include <warp/types.h>

#include <memory>
#include <vector>

namespace loomis
{
   class StateSyncUser
   {
   public:
      StateSyncUser(bool dryRun,
                    std::shared_ptr<warp::ApiManager> apiManager,
                    ServiceLogger logger);
      ~StateSyncUser() = default;

      void Sync(const warp::TracearrHistoryItem& historyItem);

   private:
      void SyncPlexState(const warp::TracearrUser& user, const warp::TracearrHistoryItem& historyItem);
      void SyncEmbyState(const warp::TracearrUser& user, const warp::TracearrHistoryItem& historyItem);

      struct LogSyncData
      {
         std::string_view server;
         std::string_view user;
         std::string_view name;
         bool watched{false};
         int32_t playbackPercentage{0};
         std::string_view syncResults;
      };
      void LogSyncSummary(const LogSyncData& syncSummary);

      template <typename ApiPtr>
      void SyncStateInternal(const warp::TracearrUser& user,
                             const warp::TracearrHistoryItem& historyItem,
                             ApiPtr sourceApi);

      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger logger_;
      std::string dryRunText_;
      warp::TracearrApi* tracearrApi_{nullptr};

      StateSyncPlexUser plexUser_;
      StateSyncEmbyUser embyUser_;
   };

   template <typename ApiPtr>
   void StateSyncUser::SyncStateInternal(const warp::TracearrUser& user,
                                         const warp::TracearrHistoryItem& historyItem,
                                         ApiPtr sourceApi)
   {
      if (!sourceApi || !sourceApi->GetValid())
         return;

      auto itemPath = sourceApi->GetItemPath(historyItem.serverRatingKey);
      if (!itemPath)
         return;

      std::string syncServers;

      for (const auto& account : user.accounts)
      {
         auto serverName = tracearrApi_->GetServerNameFromId(account.serverId);

         // If this is the server that watched the item, skip syncing
         if (account.serverId == historyItem.serverId || !serverName)
            continue;

         if (account.serverType == warp::TracearrServerType::PLEX)
         {
            auto* targetApi = apiManager_->GetTracearrPlexApi(*serverName);
            if (!targetApi)
               continue;

            StateSyncPlexUser::PlexSyncState syncState{
              .item = historyItem,
              .mediaPath = ReplaceMediaPath(*itemPath, sourceApi->GetMediaPath(), targetApi->GetMediaPath()),
              .api = targetApi,
              .userName = account.externalUserName
            };
            plexUser_.SyncState(syncState, syncServers);
         }
         else if (account.serverType == warp::TracearrServerType::EMBY)
         {
            auto* targetApi = apiManager_->GetTracearrEmbyApi(*serverName);
            if (!targetApi)
               continue;

            StateSyncEmbyUser::EmbySyncState syncState{
               .item = historyItem,
               .mediaPath = ReplaceMediaPath(*itemPath, sourceApi->GetMediaPath(), targetApi->GetMediaPath()),
               .embyApi = targetApi,
               .embyUserId = account.externalUserId
            };
            embyUser_.SyncState(syncState, syncServers);
         }
         else
         {
            //*type not supported*//
         }
      }

      // If syncServers is not empty, log the summary of the sync
      if (!syncServers.empty())
      {
         LogSyncSummary({
            .server = sourceApi->GetPrettyName(),
            .user = historyItem.user.name,
            .name = historyItem.fullName,
            .watched = historyItem.watched,
            .playbackPercentage = historyItem.playbackPercentage.value_or(0),
            .syncResults = syncServers
         });
      }
   }
}