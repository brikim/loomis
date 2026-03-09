#include "delete-files-by-age.h"

#include "services/service-types.h"

#include <warp/api/api-plex.h>
#include <warp/log/log-utils.h>
#include <warp/utils.h>

namespace loomis
{
   DeleteFilesByAge::DeleteFilesByAge(const FscDeleteFilesByAgeConfig& config,
                                        std::shared_ptr<warp::ApiManager> apiManager,
                                        ServiceLogger logger,
                                        bool dryRun)
      : apiManager_(apiManager)
      , logger_(logger)
      , dryRun_(dryRun)
      , config_(config)
   {
      Init(config);
   }

   void DeleteFilesByAge::Init(const FscDeleteFilesByAgeConfig& config)
   {
      if (dryRun_)
      {
         logger_.LogInfo("DRY RUN MODE ENABLED - No folders will be physically removed.");
      }

      namespace fs = std::filesystem;
      for (const auto& pathEntry : config.paths)
      {
         if (!fs::exists(pathEntry.path))
         {
            logger_.LogWarning("Delete file path does not exist: {}", warp::GetTag("path", pathEntry.path.string()));
            continue;
         }

         PathDeleteConfig pathDelete;
         pathDelete.path = pathEntry.path;
         pathDelete.deleteAgeHours = pathEntry.deleteAgeHours;

         for (auto& ext : pathEntry.validExtensions)
         {
            if (ext.extension.starts_with('.'))
               pathDelete.validFileExtensions.insert(warp::ToLower(ext.extension));
            else
               pathDelete.validFileExtensions.insert("." + warp::ToLower(ext.extension));
         }

         pathDeletes_.emplace_back(std::move(pathDelete));
      }
   }

   void DeleteFilesByAge::CheckForDeletes(const PathDeleteConfig& pathDelete,
                                          const std::chrono::system_clock::time_point& now,
                                          const std::filesystem::file_time_type& fileNow)
   {
      namespace fs = std::filesystem;
      std::error_code ec;

      auto it = fs::recursive_directory_iterator(pathDelete.path, fs::directory_options::skip_permission_denied, ec);
      if (ec)
      {
         logger_.LogError("Failed to open directory for iteration: {}", warp::GetTag("path", pathDelete.path.string()));
         return;
      }

      for (; it != fs::recursive_directory_iterator(); it.increment(ec))
      {
         if (ec)
         {
            logger_.LogWarning("Error advancing directory iterator: {}", warp::GetTag("error", ec.message()));
            ec.clear();
            continue;
         }

         const auto& entry = *it;
         if (!entry.is_regular_file(ec) || ec)
         {
            ec.clear();
            continue;
         }

         const auto& filePath = entry.path();
         if (!pathDelete.validFileExtensions.empty())
         {
            auto fileExt = warp::ToLower(filePath.extension().string());
            if (pathDelete.validFileExtensions.find(fileExt) == pathDelete.validFileExtensions.end())
            {
               continue;
            }
         }

         auto lastWriteTime = entry.last_write_time(ec);
         if (ec)
         {
            ec.clear();
            continue;
         }

         auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            lastWriteTime - fileNow + now);

         auto ageInHours = std::chrono::duration_cast<std::chrono::hours>(now - systemTime).count();
         if (ageInHours < pathDelete.deleteAgeHours)
         {
            continue;
         }

         if (dryRun_)
         {
            logger_.LogInfo("DRY RUN: File would be deleted: {} {}",
                            warp::GetTag("path", filePath.string()),
                            warp::GetTag("ageHours", ageInHours));
         }
         else
         {
            if (fs::remove(filePath, ec))
            {
               logger_.LogInfo("Deleted old file: {} {}",
                               warp::GetTag("path", filePath.string()),
                               warp::GetTag("ageHours", ageInHours));
            }
            else
            {
               // If file was already deleted by something else, ec will be set.
               logger_.LogError("Failed to delete file: {} {}",
                                warp::GetTag("path", filePath.string()),
                                warp::GetTag("error", ec.message()));
               ec.clear();
            }
         }
      }
   }

   void DeleteFilesByAge::Run()
   {
      auto now = std::chrono::system_clock::now();
      auto fileNow = std::filesystem::file_time_type::clock::now();
      for (const auto& pathDelete : pathDeletes_)
      {
         CheckForDeletes(pathDelete, now, fileNow);
      }
   }
}