#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-base.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-manager.h>

#include <vector>

namespace loomis
{
   class EmbyTidyService : public ServiceBase
   {
   public:
      EmbyTidyService(const EmbyTidyConfig& config,
                      std::shared_ptr<warp::ApiManager> apiManager);
      ~EmbyTidyService() = default;

      void Run() override;

   private:
      struct ServerConfig
      {
         warp::EmbyApi* api{nullptr};
         bool cleanUpNonLocalBackdrops;
         std::filesystem::path pathPrefix;
         std::vector<std::string> libraries;
      };

      void Init(const EmbyTidyConfig& config);

      void CleanNonLocalBackdrops(const ServerConfig& config);

      bool dryRun_{false};
      std::vector<ServerConfig> servers_;
   };

} // namespace Loomis