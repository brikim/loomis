#pragma once

#include "services/service-base.h"
#include "services/tmdb-sync/tmdb-playlist.h"

#include <warp/api/api-manager.h>

#include "config-reader/config-reader-types.h"

#include <memory>
#include <vector>

namespace loomis
{
   class TmdbSyncService : public ServiceBase
   {
   public:
      TmdbSyncService(const TmdbSyncConfig& config,
                      std::shared_ptr<warp::ApiManager> apiManager);
      ~TmdbSyncService() override = default;

      void Run() override;

   private:
      void Init(const TmdbSyncConfig& config);

      std::vector<std::unique_ptr<TmdbPlaylist>> playlists_;
   };
} // namespace loomis
