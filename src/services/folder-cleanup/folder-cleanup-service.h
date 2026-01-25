#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-base.h"

#include <warp/api/api-manager.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>

namespace loomis
{
   class FolderCleanupService : public ServiceBase
   {
   public:
      FolderCleanupService(const FolderCleanupConfig& config,
                            std::shared_ptr<warp::ApiManager> apiManager);
      virtual ~FolderCleanupService() = default;

      void Run() override;

   private:
      void Init(const FolderCleanupConfig& config);

      void NotifyServers(const FolderCleanupPathToCheck& pathConfig);
      bool IsFolderEmpty(const std::filesystem::path& p) const;
      void CheckFolder(const FolderCleanupPathToCheck& pathConfig);

      bool CheckMediaConnectionsValid(const std::vector<ServerLibraryConfig>& plex,
                                      const std::vector<ServerLibraryConfig>& emby);

      FolderCleanupConfig config_;
      std::unordered_set<std::filesystem::path::string_type> ignoreFolders_;
      std::unordered_set<std::filesystem::path::string_type> ignoreFiles_;
   };
}