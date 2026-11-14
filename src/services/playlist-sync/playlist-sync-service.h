#pragma once

#include "services/service-base.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-manager.h>
#include <warp/api/api-plex.h>

#include "config-reader/config-reader-types.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace loomis
{
   class PlaylistSyncService : public ServiceBase
   {
   public:
      PlaylistSyncService(const PlaylistSyncConfig& config,
                          std::shared_ptr<warp::ApiManager> apiManager);
      ~PlaylistSyncService() override = default;

      void Run() override;

   private:
      void Init(const PlaylistSyncConfig& config);

      // Returns added then deleted item numbers in the pair
      std::pair<size_t, size_t> AddRemoveEmbyPlaylistItems(warp::EmbyApi* embyApi, const warp::EmbyPlaylist& currentPlaylist, const std::vector<std::string>& updatedPlaylistIds);
      void UpdateEmbyPlaylist(warp::PlexApi* plexApi, warp::EmbyApi* embyApi, warp::EmbyPlaylist embyPlaylist, const std::vector<std::string>& correctIds);
      void SyncEmbyPlaylist(warp::PlexApi* plexApi, warp::EmbyApi* embyApi, const warp::PlexCollection& plexCollection);
      void SyncPlexCollection(warp::PlexApi* plexApi, warp::EmbyApi* embyApi, const PlaylistPlexCollection& collection);

      uint32_t timeForEmbyUpdateSec_{1u};
      std::vector<PlaylistPlexCollection> plexCollections_;
   };
} // namespace loomis
