#pragma once

#include "config-reader/config-reader-types.h"
#include "services/service-logger.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-manager.h>
#include <warp/api/api-plex.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace loomis
{
   enum class DvrActionType
   {
      KEEP_LAST,
      KEEP_NUMBER_OF_DAYS
   };

   struct DvrAction
   {
      std::string name;
      DvrActionType type;
      int32_t value;
   };

   class DvrLibrary
   {
   public:
      DvrLibrary(const DvrMaintainerLibrary& config,
                 std::shared_ptr<warp::ApiManager> apiManager,
                 ServiceLogger serviceLogger,
                 bool dryRun);
      virtual ~DvrLibrary() = default;

      DvrLibrary(const DvrLibrary&) = delete;
      DvrLibrary& operator=(const DvrLibrary&) = delete;

      [[nodiscard]] bool IsValid() const;

      void Run();

   private:
      void Init(const DvrMaintainerLibrary& config);

      [[nodiscard]] bool ServersValid();
      void DeleteItem(const std::filesystem::path& pathFileName);

      struct FileInfo
      {
         std::filesystem::path path;
         double ageDays;
      };
      std::vector<FileInfo> GetFilesInPath(std::string_view path);
      bool KeepLastDelete(std::string_view path, int32_t value);
      bool KeepDaysDelete(std::string_view path, int32_t value);
      [[nodiscard]] bool CheckDelete(DvrAction& action);

      void NotifyServers();

      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger serviceLogger_;
      std::filesystem::path path_;
      bool valid_{false};

      bool dryRun_{false};

      struct PlexApiData
      {
         warp::PlexApi* api;
         std::string libraryName;
         std::string libraryId;
      };
      std::vector<PlexApiData> plexDatas_;

      struct EmbyApiData
      {
         warp::EmbyApi* api;
         std::string libraryName;
         std::string libraryId;
      };
      std::vector<EmbyApiData> embyDatas_;

      std::vector<DvrAction> actions_;
      std::unordered_set<std::string> extensionsToDelete_;
   };
}