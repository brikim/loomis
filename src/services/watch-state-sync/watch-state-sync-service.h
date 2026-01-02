#pragma once

#include "api/api-manager.h"
#include "config-reader/config-reader-types.h"
#include "services/service-base.h"
#include "services/watch-state-sync/emby-user.h"
#include "services/watch-state-sync/plex-user.h"
#include "services/watch-state-sync/watch-state-user.h"

#include "types.h"

#include <optional>
#include <string>

namespace loomis
{
   class WatchStateSyncService : public ServiceBase
   {
   public:
      WatchStateSyncService(const WatchStateSyncConfig& config,
                          std::shared_ptr<ApiManager> apiManager);
      virtual ~WatchStateSyncService() = default;

      void Run() override;

   private:
      void Init(const WatchStateSyncConfig& config);

      std::vector<std::unique_ptr<WatchStateUser>> users_;
   };
}