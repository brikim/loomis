#pragma once

#include "services/filesystem-cleanup/delete-files-by-age.h"
#include "services/filesystem-cleanup/empty-folder-delete.h"
#include "services/service-base.h"

#include <warp/api/api-manager.h>

#include "config-reader/config-reader-types.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>

namespace loomis
{
   class FilesystemCleanupService : public ServiceBase
   {
   public:
      FilesystemCleanupService(const FileSystemCleanupConfig& config,
                               std::shared_ptr<warp::ApiManager> apiManager);
      ~FilesystemCleanupService() override = default;

      void Run() override;

   private:
      EmptyFolderDelete emptyFolderDelete_;
      DeleteFilesByAge deleteFilesByAge_;
   };
} // namespace loomis
