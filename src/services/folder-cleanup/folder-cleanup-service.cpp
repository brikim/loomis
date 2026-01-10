#include "folder-cleanup-service.h"

#include "logger/log-utils.h"

namespace loomis
{
   FolderCleanupService::FolderCleanupService(const FolderCleanupConfig& config,
                                            std::shared_ptr<ApiManager> apiManager)
      : ServiceBase("Folder Cleanup", utils::ANSI_CODE_SERVICE_FOLDER_CLEANUP, apiManager, config.cron)
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

      for (const auto& pathEntry : config.pathsToCheck)
      {
         if (!fs::exists(pathEntry.path))
         {
            LogWarning("Cleanup path does not exist: {}", utils::GetTag("path", pathEntry.path));
         }

         for (const auto& plex : pathEntry.plex)
         {
            auto* api = GetApiManager()->GetPlexApi(plex.server);
            if (!api)
            {
               LogWarning("{} api not found for {}",
                          utils::GetFormattedPlex(),
                          utils::GetTag("server", plex.server));
            }
            else if (api->GetValid() && !api->GetLibraryId(plex.libraryName))
            {
               LogWarning("{} library {} not found",
                          utils::GetServerName(utils::GetFormattedPlex(), plex.server),
                          utils::GetTag("library", plex.libraryName));
            }
         }

         for (const auto& emby : pathEntry.emby)
         {
            auto* api = GetApiManager()->GetEmbyApi(emby.server);
            if (!api)
            {
               LogWarning("{} api not found for {}",
                          utils::GetFormattedEmby(),
                          utils::GetTag("server", emby.server));
            }
            else if (api->GetValid() && !api->GetLibraryId(emby.libraryName))
            {
               LogWarning("{} library {} not found",
                          utils::GetServerName(utils::GetFormattedEmby(), emby.server),
                          utils::GetTag("library", emby.libraryName));
            }
         }
      }

      // Pre-lowercase the ignore lists for faster comparison in IsFolderEmpty
      for (auto& item : config_.ignoreFileEmptyCheck)
      {
         item.item = utils::ToLower(item.item);
      }
      for (auto& item : config_.ignoreFolders)
      {
         item.item = utils::ToLower(item.item);
      }
   }

   bool FolderCleanupService::IsFolderEmpty(const fs::path& p)
   {
      try
      {
         for (const auto& entry : fs::directory_iterator(p))
         {
            std::string name = entry.path().filename().string();
            std::string lowerName = utils::ToLower(name);

            // Skip hidden files/folders (starting with '.')
            if (!lowerName.empty() && lowerName[0] == '.') continue;

            // Is it a file we should ignore?
            bool isIgnored = std::any_of(config_.ignoreFileEmptyCheck.begin(), config_.ignoreFileEmptyCheck.end(),
               [&lowerName](const auto& item) { return lowerName == item.item; });
            if (isIgnored) continue;

            // Is it a folder we should ignore?
            bool isIgnoredDir = std::any_of(config_.ignoreFolders.begin(),
                                            config_.ignoreFolders.end(),
                                            [&lowerName](const auto& item) {return lowerName == item.item; });
            if (isIgnoredDir) continue;

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
            auto libraryId = plexApi->GetLibraryId(plexConfig.libraryName);
            if (!libraryId) continue;

            // Assuming Library Refresh is the intended notification
            plexApi->SetLibraryScan(*libraryId);

            syncServerNames = utils::BuildSyncServerString(syncServerNames, utils::GetFormattedPlex(), plexConfig.server) + ":" + plexConfig.libraryName;
         }
      }

      // Notify Emby Servers
      for (const auto& embyConfig : pathConfig.emby)
      {
         if (auto* embyApi = GetApiManager()->GetEmbyApi(embyConfig.server);
             embyApi && embyApi->GetValid())
         {
            auto libraryId = embyApi->GetLibraryId(embyConfig.libraryName);
            if (!libraryId) continue;

            embyApi->SetLibraryScan(*libraryId);

            syncServerNames = utils::BuildSyncServerString(syncServerNames, utils::GetFormattedEmby(), embyConfig.server) + ":" + embyConfig.libraryName;
         }
      }

      if (!syncServerNames.empty())
      {
         LogInfo("Notifying {} of folder deletion", syncServerNames);
      }
   }

   bool FolderCleanupService::CheckMediaConnectionsValid(const std::vector<FolderCleanupServerConfig>& plex,
                                                         const std::vector<FolderCleanupServerConfig>& emby)
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
         LogWarning("Skipping cleanup for {} - one or more servers are offline", utils::GetTag("path", pathConfig.path));
         return;
      }

      fs::path rootPath(pathConfig.path);
      if (!fs::exists(rootPath)) return;

      bool directoryDeleted = false;
      std::vector<fs::path> subdirs;

      try
      {
         // Collect all subdirectories inside the top-level path
         for (const auto& entry : fs::recursive_directory_iterator(rootPath))
         {
            if (entry.is_directory())
            {
               subdirs.push_back(entry.path());
            }
         }
      }
      catch (const std::exception& e)
      {
         LogWarning("Failed to iterate {}: {}", utils::GetTag("path", rootPath.string()), e.what());
         return;
      }

      // Sort by depth (deepest first). 
      // This way if 'Root/Folder/Subfolder' are all empty:
      // 1. Subfolder is deleted.
      // 2. Folder is now empty and deleted.
      // 3. Root is checked but we skip it.
      std::sort(subdirs.begin(), subdirs.end(), [](const fs::path& a, const fs::path& b) {
         return a.string().length() > b.string().length();
      });

      for (const auto& dir : subdirs)
      {
         // Safety: Never delete the top-level path itself
         if (dir == rootPath) continue;

         if (IsFolderEmpty(dir))
         {
            if (config_.dryRun)
            {
               // We log what WE WOULD HAVE done
               LogInfo("[Dry Run] Would remove empty folder: {}",
                       utils::GetTag("path", utils::GetStandoutText(dir.string())));

               directoryDeleted = false;
            }
            else
            {
               std::error_code ec;
               if (fs::remove(dir, ec))
               {
                  LogInfo("Removed empty folder: {}", utils::GetTag("path", utils::GetStandoutText(dir.string())));
                  directoryDeleted = true;
               }
               else if (ec)
               {
                  LogWarning("Failed to remove {}: {}", utils::GetTag("path", dir.string()), ec.message());
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