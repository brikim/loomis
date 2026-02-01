#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-base.h"
#include "services/watch-state-sync/emby-user.h"
#include "services/watch-state-sync/plex-user.h"
#include "services/watch-state-sync/watch-state-user.h"

#include <warp/api//api-manager.h>

#include <memory>
#include <optional>
#include <string>

namespace loomis
{
   class WatchStateSyncService : public ServiceBase
   {
   public:
      WatchStateSyncService(const WatchStateSyncConfig& config,
                            std::shared_ptr<warp::ApiManager> apiManager);
      ~WatchStateSyncService() = default;

      void Run() override;

   private:
      void Init(const WatchStateSyncConfig& config);

      std::vector<std::unique_ptr<WatchStateUser>> users_;
   };
}