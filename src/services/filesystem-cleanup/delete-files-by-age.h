#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"

#include <warp/api/api-manager.h>

#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <unordered_set>

namespace loomis
{
   class DeleteFilesByAge
   {
   public:
      DeleteFilesByAge(const FscDeleteFilesByAgeConfig& config,
                        std::shared_ptr<warp::ApiManager> apiManager,
                        ServiceLogger logger,
                        bool dryRun);
      ~DeleteFilesByAge() = default;

      void Run();

   private:
      struct PathDeleteConfig
      {
         std::filesystem::path path;
         int64_t deleteAgeHours{std::numeric_limits<int64_t>::max()};
         std::unordered_set<std::filesystem::path> validFileExtensions;
      };

      void Init(const FscDeleteFilesByAgeConfig& config);
      void CheckForDeletes(const PathDeleteConfig& pathDelete,
                           const std::chrono::system_clock::time_point& now,
                           const std::filesystem::file_time_type& fileNow);

      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger logger_;
      bool dryRun_{false};

      FscDeleteFilesByAgeConfig config_;
      std::vector<PathDeleteConfig> pathDeletes_;
   };
}