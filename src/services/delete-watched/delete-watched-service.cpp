#include "delete-watched-service.h"

#include "services/service-types.h"

#include <warp/log/log-utils.h>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("Delete Watched");
   }

   DeleteWatchedService::DeleteWatchedService(const DeleteWatchedConfig& config,
                                              std::shared_ptr<warp::ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_DELETE_WATCHED, apiManager, config.cron)
      , config_(config)
   {
      Init(config_);
   }

   void DeleteWatchedService::Init(const DeleteWatchedConfig& config)
   {
      if (config_.dryRun)
      {
         LogInfo("[DRY RUN] Enabled - No files will be physically removed.");
      }

      for (const auto& library : config.libraries)
      {
         auto dvrLibrary = std::make_unique<DeleteWatchedLibrary>(library,
                                                                  config.deleteTimeHours,
                                                                  GetApiManager(),
                                                                  ServiceLogger(*this),
                                                                  config.dryRun);
         if (dvrLibrary->IsValid())
         {
            libraries_.emplace_back(std::move(dvrLibrary));
         }
         else
         {
            LogError("Failed to initialize DVR library for {}", warp::GetTag("path", library.containerPath.string()));
         }
      }
   }

   void DeleteWatchedService::Run()
   {
      for (auto& library : libraries_)
      {
         library->Run();
      }
   }
}