#include "dvr-maintainer-service.h"

#include "services/service-types.h"

#include <warp/log-utils.h>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("DVR Maintainer");
   }

   DvrMaintainerService::DvrMaintainerService(const DvrMaintainerConfig& config,
                                              std::shared_ptr<ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_DVR_MAINTAINER, apiManager, config.cron)
      , config_(config)
   {
      Init(config_);
   }

   void DvrMaintainerService::Init(const DvrMaintainerConfig& config)
   {
      if (config_.dryRun)
      {
         LogInfo("DRY RUN MODE ENABLED - No files will be physically removed.");
      }

      for (const auto& library : config.libraries)
      {
         auto dvrLibrary = std::make_unique<DvrLibrary>(library, GetApiManager(), ServiceLogger(*this), config.dryRun);
         if (dvrLibrary->IsValid())
         {
            libraries_.emplace_back(std::move(dvrLibrary));
         }
         else
         {
            LogError("Failed to initialize DVR library for {}", warp::GetTag("path", library.path));
         }
      }
   }

   void DvrMaintainerService::Run()
   {
      for (auto& library : libraries_)
      {
         library->Run();
      }
   }
}