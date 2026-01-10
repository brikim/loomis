#pragma once

#include "api/api-manager.h"
#include "config-reader/config-reader-types.h"
#include "services/service-base.h"

#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;

namespace loomis
{
   class FolderCleanupService : public ServiceBase
   {
   public:
      FolderCleanupService(const FolderCleanupConfig& config,
                            std::shared_ptr<ApiManager> apiManager);
      virtual ~FolderCleanupService() = default;

      void Run() override;

   private:
      void Init(const FolderCleanupConfig& config);

      void NotifyServers(const FolderCleanupPathToCheck& pathConfig);
      bool IsFolderEmpty(const fs::path& p);
      void CheckFolder(const FolderCleanupPathToCheck& pathConfig);

      bool CheckMediaConnectionsValid(const std::vector<FolderCleanupServerConfig>& plex,
                                      const std::vector<FolderCleanupServerConfig>& emby);

      FolderCleanupConfig config_;
   };
}