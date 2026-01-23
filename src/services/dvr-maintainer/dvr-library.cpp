#include "dvr-library.h"

#include <warp/log.h>
#include <warp/log-utils.h>
#include <warp/utils.h>

namespace loomis
{
   namespace
   {
      constexpr std::string_view KEEP_LAST_VALUE("KEEP_LAST_");
      constexpr std::string_view KEEP_LENGTH_DAYS_VALUE("KEEP_LENGTH_DAYS_");
   }

   DvrLibrary::DvrLibrary(const DvrMaintainerLibrary& config,
                          std::shared_ptr<ApiManager> apiManager,
                          ServiceLogger serviceLogger,
                          bool dryRun)
      : apiManager_(apiManager)
      , serviceLogger_(serviceLogger)
      , dryRun_(dryRun)
   {
      Init(config);
   }

   void DvrLibrary::Init(const DvrMaintainerLibrary& config)
   {
      if (!apiManager_)
      {
         serviceLogger_.LogError("ApiManager is not initialized.");
         return;
      }

      // Helper lambda to reduce duplication between Plex and Emby logic
      auto processMediaServer = [&](auto& serverConfig, auto getApiFunc, auto& targetContainer, const auto& formatName) {
         if (auto api = getApiFunc(serverConfig.server); api)
         {
            std::string libraryId;
            if (api->GetValid())
            {
               auto libId = api->GetLibraryId(serverConfig.library);
               if (libId) libraryId = *libId;
            }
            targetContainer.emplace_back(api, serverConfig.library, libraryId);
         }
         else
         {
            serviceLogger_.LogWarning("No {} found with {} ... Skipping",
                formatName,
                warp::GetTag("server_name", serverConfig.server));
         }
      };

      for (const auto& plex : config.plex)
      {
         processMediaServer(plex, [&](const std::string& s) { return apiManager_->GetPlexApi(s); }, plexDatas_, warp::GetFormattedPlex());
      }

      for (const auto& emby : config.emby)
      {
         processMediaServer(emby, [&](const std::string& s) { return apiManager_->GetEmbyApi(s); }, embyDatas_, warp::GetFormattedEmby());
      }

      // Handle Path
      if (std::filesystem::exists(config.path))
      {
         path_ = config.path;
      }
      else
      {
         serviceLogger_.LogWarning("Path does not exist: {}", warp::GetTag("path", config.path));
         return;
      }

      // Handle Actions
      for (const auto& action : config.actions)
      {
         std::optional<DvrActionType> type;
         std::string_view prefix;

         if (action.action.starts_with(KEEP_LAST_VALUE))
         {
            type = DvrActionType::KEEP_LAST;
            prefix = KEEP_LAST_VALUE;
         }
         else if (action.action.starts_with(KEEP_LENGTH_DAYS_VALUE))
         {
            type = DvrActionType::KEEP_NUMBER_OF_DAYS;
            prefix = KEEP_LENGTH_DAYS_VALUE;
         }

         if (type)
         {
            auto valuePart = std::string_view(action.action).substr(prefix.size());

            int value = 0;
            auto [ptr, ec] = std::from_chars(valuePart.data(), valuePart.data() + valuePart.size(), value);

            if (ec == std::errc())
            {
               actions_.emplace_back(DvrAction{
                   .name = action.name,
                   .type = *type,
                   .value = value
               });
            }
            else if (ec == std::errc::invalid_argument)
            {
               serviceLogger_.LogError("Action '{}' does not contain a valid number", action.action);
            }
            else if (ec == std::errc::result_out_of_range)
            {
               serviceLogger_.LogError("Number in action '{}' is too large for an integer", action.action);
            }
         }
         else
         {
            serviceLogger_.LogWarning("Unknown action {}", action.action);
         }
      }

      for (auto& extToDelete : config.extensionsToDelete)
      {
         if (extToDelete.extension.starts_with('.'))
         {
            extensionsToDelete_.insert(warp::ToLower(extToDelete.extension));
         }
         else
         {
            extensionsToDelete_.insert("." + warp::ToLower(extToDelete.extension));
         }
      }

      valid_ = actions_.size() > 0 && (plexDatas_.size() + embyDatas_.size()) > 0;
   }

   bool DvrLibrary::IsValid() const
   {
      return valid_;
   }

   std::vector<DvrLibrary::FileInfo> DvrLibrary::GetFilesInPath(std::string_view path)
   {
      std::vector<FileInfo> files;
      std::error_code ec;

      if (!std::filesystem::exists(path, ec)) return files;

      auto iterOpts = std::filesystem::directory_options::skip_permission_denied;

      try
      {
         auto now = std::filesystem::file_time_type::clock::now();

         for (const auto& entry : std::filesystem::recursive_directory_iterator(path, iterOpts))
         {
            const auto& p = entry.path();
            if (!entry.is_regular_file()) continue;

            if (extensionsToDelete_.count(warp::ToLower(p.extension().string())))
            {
               auto ftime = entry.last_write_time();
               auto ageDuration = now - ftime;
               using days_f = std::chrono::duration<double, std::ratio<86400>>;
               double days = std::chrono::duration_cast<days_f>(ageDuration).count();

               files.push_back({.path = p, .ageDays = days});
            }
         }
      }
      catch (const std::exception& e)
      {
         serviceLogger_.LogError("Critical error scanning path: {}", e.what());
      }

      return files;
   }

   void DvrLibrary::DeleteItem(const std::filesystem::path& pathFileName)
   {
      if (dryRun_)
      {
         serviceLogger_.LogInfo("[DRY RUN] Would delete {}",
                                warp::GetTag("file", pathFileName.filename().string()));
      }
      else
      {
         try
         {
            std::filesystem::remove(pathFileName);
         }
         catch (const std::filesystem::filesystem_error& e)
         {
            serviceLogger_.LogError("Problem deleting {} {}",
                                    warp::GetTag("file", pathFileName.string()),
                                    warp::GetTag("error", e.what()));
         }
      }
   }

   bool DvrLibrary::KeepLastDelete(std::string_view path, int32_t value)
   {
      bool showsDeleted = false;
      auto fileInfo = GetFilesInPath(path);

      if (fileInfo.size() > static_cast<size_t>(value))
      {
         serviceLogger_.LogInfo("KEEP_LAST_{} {} {}",
                                value,
                                warp::GetTag("num_items", fileInfo.size()),
                                warp::GetTag("path", warp::GetStandoutText(warp::GetDisplayFolder(path))));

         std::sort(fileInfo.begin(), fileInfo.end(), [](const FileInfo& a, const FileInfo& b) {
            return a.ageDays > b.ageDays;
         });

         size_t showsToDelete = fileInfo.size() - value;
         size_t deletedCount = 0;

         for (const auto& file : fileInfo)
         {
            serviceLogger_.LogInfo("KEEP_LAST_{} deleting oldest {} {}",
                                   value,
                                   warp::GetTag("age days", file.ageDays, ".1f"),
                                   warp::GetTag("file", warp::GetStandoutText(warp::GetDisplayFolder(file.path.string()))));


            DeleteItem(file.path);
            showsDeleted = true;

            ++deletedCount;
            if (deletedCount >= showsToDelete) break;
         }
      }
      return showsDeleted;
   }

   bool DvrLibrary::KeepDaysDelete(std::string_view path, int32_t value)
   {
      bool showsDeleted = false;
      auto fileInfo = GetFilesInPath(path);

      for (const auto& file : fileInfo)
      {
         if (file.ageDays >= static_cast<double>(value))
         {
            serviceLogger_.LogInfo("KEEP_DAYS_{} deleting {} {}",
                                   value,
                                   warp::GetTag("age days", file.ageDays, ".1f"),
                                   warp::GetTag("file", warp::GetStandoutText(warp::GetDisplayFolder(file.path.string()))));

            DeleteItem(file.path);
            showsDeleted = true;
         }
      }
      return showsDeleted;
   }

   bool DvrLibrary::CheckDelete(DvrAction& action)
   {
      auto libraryFilePath = path_ / action.name;
      if (!std::filesystem::exists(libraryFilePath)) return false;

      switch (action.type)
      {
         case DvrActionType::KEEP_LAST:
            return KeepLastDelete(libraryFilePath.string(), action.value);
         case DvrActionType::KEEP_NUMBER_OF_DAYS:
            return KeepDaysDelete(libraryFilePath.string(), action.value);
         default:
            return false;
      }
   }

   void DvrLibrary::NotifyServers()
   {
      std::string syncServerNames;

      // Notify Plex Servers
      for (auto& plexData : plexDatas_)
      {
         if (!dryRun_) plexData.api->SetLibraryScan(plexData.libraryId);

         syncServerNames = warp::BuildSyncServerString(syncServerNames, plexData.api->GetPrettyName(), "") + ":" + plexData.libraryName;
      }

      // Notify Emby Servers
      for (auto& embyData : embyDatas_)
      {
         if (!dryRun_) embyData.api->SetLibraryScan(embyData.libraryId);

         syncServerNames = warp::BuildSyncServerString(syncServerNames, embyData.api->GetPrettyName(), "") + ":" + embyData.libraryName;
      }

      if (!syncServerNames.empty())
      {
         serviceLogger_.LogInfo("Notified {} to refresh", syncServerNames);
      }
   }

   bool DvrLibrary::ServersValid()
   {
      auto validateGroup = [&](auto& dataContainer) {
         for (auto& data : dataContainer)
         {
            if (!data.api->GetValid())
            {
               serviceLogger_.LogWarning("{} not available",
                   data.api->GetPrettyName());
               return false;
            }

            if (data.libraryId.empty())
            {
               auto libId = data.api->GetLibraryId(data.libraryName);
               if (!libId)
               {
                  serviceLogger_.LogWarning("{} {} not valid",
                      data.api->GetPrettyName(),
                      warp::GetTag("library", data.libraryName));
                  return false;
               }
               data.libraryId = *libId;
            }
         }
         return true;
      };

      if (!validateGroup(plexDatas_)) return false;
      if (!validateGroup(embyDatas_)) return false;

      return true;
   }

   void DvrLibrary::Run()
   {
      if (!ServersValid())
      {
         serviceLogger_.LogWarning("Skipping Run");
         return;
      }

      bool itemsDeleted = false;
      for (auto& action : actions_)
      {
         itemsDeleted |= CheckDelete(action);
      }

      if (itemsDeleted) NotifyServers();
   }
}