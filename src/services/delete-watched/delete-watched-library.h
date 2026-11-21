#pragma once

#include "services/delete-watched/delete-watched-types.h"
#include "services/service-logger.h"

#include <warp/api/api-tracearr-types.h>

#include "config-reader/config-reader-types.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace warp
{
   class ApiManager;
   class TracearrApi;
} // namespace warp

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
      ~DeleteWatchedLibrary() = default;

      DeleteWatchedLibrary(const DeleteWatchedLibrary&) = delete;
      DeleteWatchedLibrary& operator=(const DeleteWatchedLibrary&) = delete;

      [[nodiscard]] bool IsValid() const;

      void ProcessDeletes(std::string_view user, const warp::TracearrHistoryItems& userHistory);

   private:
      void Init(const DeleteWatchedLibraryConfig& config);

      std::optional<DeleteFileInfo> ValidPlexItem(const std::string_view user, const warp::TracearrHistoryItem& historyItem);
      std::optional<DeleteFileInfo> ValidEmbyItem(const std::string_view user, const warp::TracearrHistoryItem& historyItem);

      [[nodiscard]] bool DeleteFiles(const std::vector<DeleteFileInfo>& files);

      void NotifyServers();

      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger serviceLogger_;
      std::filesystem::path containerPath_;
      bool valid_{false};
      std::chrono::hours deleteTimeHours_;

      bool dryRun_{false};
      std::string_view dryRunText_;

      std::vector<DeleteWatchedPlexData> plexDatas_;
      std::vector<DeleteWatchedEmbyData> embyDatas_;
   };
} // namespace loomis
