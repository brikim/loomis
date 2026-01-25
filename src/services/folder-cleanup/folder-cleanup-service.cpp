#include "folder-cleanup-service.h"

#include "services/service-types.h"

#include <warp/api/api-plex.h>
#include <warp/log/log-utils.h>
#include <warp/utils.h>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("Folder Cleanup");
   }

   FolderCleanupService::FolderCleanupService(const FolderCleanupConfig& config,
                                            std::shared_ptr<warp::ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_FOLDER_CLEANUP, apiManager, config.cron)
      , config_(config)
   {
      Init(config);
   }

   void FolderCleanupService::Init(const FolderCleanupConfig& config)
   {
      if (config_.dryRun)
      {
         LogInfo("DRY RUN MODE ENABLED - No folders will be physically removed.");
      }

      namespace fs = std::filesystem;
      for (const auto& pathEntry : config.pathsToCheck)
      {
         if (!fs::exists(pathEntry.path))
         {
            LogWarning("Cleanup path does not exist: {}", warp::GetTag("path", pathEntry.path));
         }

         for (const auto& plex : pathEntry.plex)
         {
            if (auto* api = GetApiManager()->GetPlexApi(plex.server); !api)
            {
               LogWarning("{} api not found for {}",
                          warp::GetFormattedPlex(),
                          warp::GetTag("server", plex.server));
            }
            else if (api->GetValid() && !api->GetLibraryId(plex.library))
            {
               LogWarning("{} library {} not found",
                          api->GetPrettyName(),
                          warp::GetTag("library", plex.library));
            }
         }

         for (const auto& emby : pathEntry.emby)
         {
            if (auto* api = GetApiManager()->GetEmbyApi(emby.server); !api)
            {
               LogWarning("{} api not found for {}",
                          warp::GetFormattedEmby(),
                          warp::GetTag("server", emby.server));
            }
            else if (api->GetValid() && !api->GetLibraryId(emby.library))
            {
               LogWarning("{} library {} not found",
                          api->GetPrettyName(),
                          warp::GetTag("library", emby.library));
            }
         }
      }

      // Pre-lowercase the ignore lists for faster comparison in IsFolderEmpty
      for (auto& item : config_.ignoreFileEmptyCheck)
      {
         ignoreFiles_.emplace(warp::ToLower(fs::path(item.item).native()));
      }
      for (auto& item : config_.ignoreFolders)
      {
         ignoreFolders_.emplace(warp::ToLower(fs::path(item.item).native()));
      }
   }

   bool FolderCleanupService::IsFolderEmpty(const std::filesystem::path& p) const
   {
      try
      {
         namespace fs = std::filesystem;
         for (const auto& entry : fs::directory_iterator(p))
         {
            const auto& path = entry.path();
            auto name = path.filename().native();
            if (!name.empty() && name[0] == '.') continue;

            if (auto lowerName = warp::ToLower(name);
                ignoreFiles_.contains(lowerName) || ignoreFolders_.contains(lowerName))
               continue;

            // If we found something that isn't ignored, the folder isn't empty
            return false;
         }
      }
      catch (...)
      {
         return false;
      }

      return true; // No non-ignored items found
   }

   void FolderCleanupService::NotifyServers(const FolderCleanupPathToCheck& pathConfig)
   {
      std::string syncServerNames;

      // Notify Plex Servers
      for (const auto& plexConfig : pathConfig.plex)
      {
         if (auto* plexApi = GetApiManager()->GetPlexApi(plexConfig.server);
             plexApi && plexApi->GetValid())
         {
            auto libraryId = plexApi->GetLibraryId(plexConfig.library);
            if (!libraryId) continue;

            // Assuming Library Refresh is the intended notification
            plexApi->SetLibraryScan(*libraryId);

            syncServerNames = warp::BuildSyncServerString(syncServerNames, warp::GetFormattedPlex(), plexConfig.server) + ":" + plexConfig.library;
         }
      }

      // Notify Emby Servers
      for (const auto& embyConfig : pathConfig.emby)
      {
         if (auto* embyApi = GetApiManager()->GetEmbyApi(embyConfig.server);
             embyApi && embyApi->GetValid())
         {
            auto libraryId = embyApi->GetLibraryId(embyConfig.library);
            if (!libraryId) continue;

            embyApi->SetLibraryScan(*libraryId);

            syncServerNames = warp::BuildSyncServerString(syncServerNames, warp::GetFormattedEmby(), embyConfig.server) + ":" + embyConfig.library;
         }
      }

      if (!syncServerNames.empty())
      {
         LogInfo("Notifying {} of folder deletion", syncServerNames);
      }
   }

   bool FolderCleanupService::CheckMediaConnectionsValid(const std::vector<ServerLibraryConfig>& plex,
                                                         const std::vector<ServerLibraryConfig>& emby)
   {
      for (const auto& p : plex)
      {
         auto* api = GetApiManager()->GetPlexApi(p.server);
         if (!api || !api->GetValid()) return false;
      }
      for (const auto& e : emby)
      {
         auto* api = GetApiManager()->GetEmbyApi(e.server);
         if (!api || !api->GetValid()) return false;
      }
      return true;
   }

   void FolderCleanupService::CheckFolder(const FolderCleanupPathToCheck& pathConfig)
   {
      if (!CheckMediaConnectionsValid(pathConfig.plex, pathConfig.emby))
      {
         LogWarning("Skipping cleanup for {} - one or more servers are offline", warp::GetTag("path", pathConfig.path));
         return;
      }

      namespace fs = std::filesystem;
      fs::path rootPath(pathConfig.path);
      if (!fs::exists(rootPath)) return;

      struct PathEntry
      {
         fs::path path;
         size_t depth;
      };
      bool directoryDeleted = false;
      std::vector<PathEntry> subdirs;
      std::error_code ec;

      // Use error_code to avoid exceptions on permission-denied subfolders
      for (auto it = fs::recursive_directory_iterator(rootPath, ec); it != fs::recursive_directory_iterator(); ++it)
      {
         if (ec)
         {
            LogWarning("Error accessing {}: {}", it->path().string(), ec.message());
            ec.clear();
            continue;
         }
         if (it->is_directory()) subdirs.push_back({it->path(), static_cast<size_t>(it.depth())});
      }

      // Robust depth sort: Deepest paths (most components) first
      std::sort(subdirs.begin(), subdirs.end(), [](const auto& a, const auto& b) {
         return a.depth > b.depth;
      });

      for (const auto& dir : subdirs)
      {
         // Safety: Never delete the top-level path itself
         if (dir.path == rootPath) continue;

         if (IsFolderEmpty(dir.path))
         {
            if (config_.dryRun)
            {
               LogInfo("[Dry Run] Would remove empty folder: {}",
                       warp::GetTag("path", warp::GetStandoutText(dir.path.string())));
            }
            else
            {
               if (std::error_code ec; fs::remove_all(dir.path, ec))
               {
                  LogInfo("Removed empty folder: {}", warp::GetTag("path", warp::GetStandoutText(dir.path.string())));
                  directoryDeleted = true;
               }
               else if (ec)
               {
                  LogWarning("Failed to remove {}: {}", warp::GetTag("path", dir.path.string()), ec.message());
               }
            }
         }
      }

      if (directoryDeleted)
      {
         NotifyServers(pathConfig);
      }
   }

   void FolderCleanupService::Run()
   {
      for (const auto& pathConfig : config_.pathsToCheck)
      {
         CheckFolder(pathConfig);
      }
   }
}