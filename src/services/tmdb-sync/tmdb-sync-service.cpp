#include "tmdb-sync-service.h"

#include "services/service-logger.h"
#include "services/service-types.h"
#include "services/service-utils.h"

#include <warp/log/log-utils.h>

#include <algorithm>


namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("Tmdb Sync");
   }

   TmdbSyncService::TmdbSyncService(const TmdbSyncConfig& config,
                                    std::shared_ptr<warp::ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_TMDB_SYNC, apiManager, config.cron)
   {
      Init(config);
   }

   void TmdbSyncService::Init(const TmdbSyncConfig& config)
   {
      for (const auto& playlistConfig : config.playlists)
      {
         if (auto playlist = std::make_unique<TmdbPlaylist>(playlistConfig, config.dryRun, GetApiManager(), ServiceLogger(*this));
             playlist->IsValid())
         {
            playlists_.emplace_back(std::move(playlist));
         }
      }
   }

   void TmdbSyncService::Run()
   {
      std::ranges::for_each(playlists_, [](const auto& playlist) {
         playlist->Sync();
      });
   }
} // namespace loomis
