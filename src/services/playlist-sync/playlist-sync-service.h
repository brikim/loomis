#pragma once

#include "api/api-manager.h"
#include "config-reader/config-reader-types.h"
#include "services/service-base.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace loomis
{
   class PlaylistSyncService : public ServiceBase
   {
   public:
      PlaylistSyncService(const PlaylistSyncConfig& config,
                          std::shared_ptr<ApiManager> apiManager);
      virtual ~PlaylistSyncService() = default;

      void Run() override;

   private:
      void Init(const PlaylistSyncConfig& config);

      // Returns added then deleted item numbers in the pair
      std::pair<size_t, size_t> AddRemoveEmbyPlaylistItems(EmbyApi* embyApi, const EmbyPlaylist& currentPlaylist, const std::vector<std::string>& updatedPlaylistIds);
      bool UpdateToCorrectLocation(EmbyApi* embyApi, const EmbyPlaylist& currentPlaylist, std::string_view id, uint32_t correctIndex);
      void UpdateEmbyPlaylist(PlexApi* plexApi, EmbyApi* embyApi, std::string_view playlistName, const std::vector<std::string>& correctIds);
      void SyncEmbyPlaylist(PlexApi* plexApi, EmbyApi* embyApi, const PlexCollection& plexCollection);
      void SyncPlexCollection(PlexApi* plexApi, EmbyApi* embyApi, const PlaylistPlexCollection& collection);

      uint32_t timeForEmbyUpdateSec_{1u};
      uint32_t timeBetweenSyncsSec_{1u};
      std::vector<PlaylistPlexCollection> plexCollections_;
   };
}