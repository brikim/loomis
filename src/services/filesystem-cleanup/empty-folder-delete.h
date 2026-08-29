#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"

#include <warp/api/api-manager.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_set>

namespace loomis
{
   class EmptyFolderDelete
   {
   public:
      EmptyFolderDelete(const FscEmptyFolderDeleteConfig& config,
                        std::shared_ptr<warp::ApiManager> apiManager,
                        ServiceLogger logger,
                        bool dryRun);
      ~EmptyFolderDelete() = default;

      void Run();

   private:
      void Init(const FscEmptyFolderDeleteConfig& config);

      void NotifyServers(const FscEmptyFolderPathConfig& pathConfig);
      bool IsFolderEmpty(const std::filesystem::path& p,
                         const std::unordered_set<std::filesystem::path>& deletedFolders) const;
      void CheckFolder(const FscEmptyFolderPathConfig& pathConfig);

      bool CheckMediaConnectionsValid(const std::vector<ServerLibraryConfig>& plex,
                                      const std::vector<ServerLibraryConfig>& emby);

      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger logger_;
      bool dryRun_{false};

      FscEmptyFolderDeleteConfig config_;
      std::unordered_set<std::filesystem::path::string_type> ignoreFolders_;
      std::unordered_set<std::filesystem::path::string_type> ignoreFiles_;
   };
}