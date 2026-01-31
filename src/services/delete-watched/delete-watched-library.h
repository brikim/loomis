#pragma once

#include "config-reader/config-reader-types.h"
#include "services/delete-watched/delete-watched-types.h"
#include "services/service-logger.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-manager.h>
#include <warp/api/api-plex.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace loomis
{
   class DeleteWatchedLibrary
   {
   public:
      DeleteWatchedLibrary(const DeleteWatchedLibraryConfig& config,
                           int32_t deleteTimeHours,
                           std::shared_ptr<warp::ApiManager> apiManager,
                           ServiceLogger serviceLogger,
                           bool dryRun);
      virtual ~DeleteWatchedLibrary() = default;

      DeleteWatchedLibrary(const DeleteWatchedLibrary&) = delete;
      DeleteWatchedLibrary& operator=(const DeleteWatchedLibrary&) = delete;

      [[nodiscard]] bool IsValid() const;

      void Run();

   private:
      void Init(const DeleteWatchedLibraryConfig& config);

      std::vector<DeleteFileInfo> FindPlexWatched(const DeleteWatchedPlexData& data,
                                                  const std::string& dataTimeForHistory,
                                                  int64_t epochDateTimeForHistory);

      std::vector<DeleteFileInfo> FindEmbyWatched(const DeleteWatchedEmbyData& data,
                                                  const std::string& dataTimeForHistory,
                                                  int64_t epochDateTimeForHistory);

      [[nodiscard]] bool DeleteFiles(const std::vector<DeleteFileInfo>& files);

      void NotifyServers();

      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger serviceLogger_;
      std::string containerPath_;
      bool valid_{false};
      std::chrono::hours deleteTimeHours_;
      int32_t historyDays_{1};

      bool dryRun_{false};
      std::string dryRunText_;

      std::vector<DeleteWatchedPlexData> plexDatas_;
      std::vector<DeleteWatchedEmbyData> embyDatas_;
   };
}