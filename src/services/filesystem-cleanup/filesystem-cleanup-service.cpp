#include "filesystem-cleanup-service.h"

#include "services/service-types.h"

#include <warp/api/api-plex.h>
#include <warp/log/log-utils.h>
#include <warp/utils.h>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("Filesystem Cleanup");
   }

   FilesystemCleanupService::FilesystemCleanupService(const FileSystemCleanupConfig& config,
                                            std::shared_ptr<warp::ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_FILESYSTEM_CLEANUP, apiManager, config.cron)
      , emptyFolderDelete_(config.emptyFolderDeleteConfig, apiManager, ServiceLogger(*this), config.dryRun)
      , deleteFilesByAge_(config.deleteFilesByAgeConfig, apiManager, ServiceLogger(*this), config.dryRun)
   {
   }

   void FilesystemCleanupService::Run()
   {
      emptyFolderDelete_.Run();
      deleteFilesByAge_.Run();
   }
}