#include "delete-watched-library.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-manager.h>
#include <warp/api/api-plex.h>
#include <warp/api/api-tracearr.h>
#include <warp/log/log-utils.h>
#include <warp/log/log.h>
#include <warp/utils.h>

#include <ranges>

namespace loomis
{
   DeleteWatchedLibrary::DeleteWatchedLibrary(const DeleteWatchedLibraryConfig& config,
                                              int32_t deleteTimeHours,
                                              std::shared_ptr<warp::ApiManager> apiManager,
                                              ServiceLogger serviceLogger,
                                              bool dryRun)
      : apiManager_(apiManager)
      , serviceLogger_(serviceLogger)
      , containerPath_(config.containerPath)
      , deleteTimeHours_(deleteTimeHours)
      , dryRun_(dryRun)
      , dryRunText_(dryRun ? "[DRY RUN] " : "")
   {
      Init(config);
   }

   void DeleteWatchedLibrary::Init(const DeleteWatchedLibraryConfig& config)
   {
      if (!apiManager_)
      {
         serviceLogger_.LogError("ApiManager is not initialized.");
         return;
      }

      for (const auto& plexConfig : config.plex)
      {
         auto* plexApi = apiManager_->GetPlexApi(plexConfig.server);
         if (!plexApi)
         {
            serviceLogger_.LogWarning(
               "No {} found with {}",
               warp::GetFormattedPlex(),
               warp::GetTag("server_name", plexConfig.server));
            continue;
         }

         bool plexLibraryValid = true;
         if (plexApi->GetValid())
         {
            if (auto libId = plexApi->GetLibraryId(plexConfig.library);
                !libId)
            {
               serviceLogger_.LogWarning(
                  "{} does not have {} ... Skipping",
                  plexApi->GetPrettyName(),
                  warp::GetTag("library", plexConfig.library));
               plexLibraryValid = false;
            }
         }

         if (plexLibraryValid)
         {
            plexDatas_.emplace_back(
               DeleteWatchedPlexData{
                  .api = plexApi,
                  .libraryName = plexConfig.library,
                  .mediaPath = plexConfig.mediaPath});
         }
      }

      for (const auto& embyConfig : config.emby)
      {
         auto* embyApi = apiManager_->GetEmbyApi(embyConfig.server);
         if (!embyApi)
         {
            serviceLogger_.LogWarning(
               "No {} found with {}",
               warp::GetFormattedEmby(),
               warp::GetTag("server_name", embyConfig.server));
            continue;
         }

         bool embyLibraryValid = true;
         if (embyApi->GetValid())
         {
            if (auto libId = embyApi->GetLibraryId(embyConfig.library);
                !libId)
            {
               embyLibraryValid = false;
               serviceLogger_.LogWarning(
                  "{} does not have {} ... Skipping",
                  embyApi->GetPrettyName(),
                  warp::GetTag("library", embyConfig.library));
            }
         }

         if (embyLibraryValid)
         {
            embyDatas_.emplace_back(
               DeleteWatchedEmbyData{
                  .api = embyApi,
                  .libraryName = embyConfig.library,
                  .mediaPath = embyConfig.mediaPath});
         }
      }

      valid_ = (plexDatas_.size() + embyDatas_.size()) > 0;
   }

   bool DeleteWatchedLibrary::IsValid() const
   {
      return valid_;
   }

   std::optional<DeleteFileInfo> DeleteWatchedLibrary::ValidPlexItem(const std::string_view user,
                                                                     const warp::TracearrHistoryItem& historyItem)
   {
      for (const auto& plexData : plexDatas_)
      {
         if ((historyItem.serverName != plexData.api->GetTracearrServerName()) ||
             !plexData.api->GetValid())
            continue;

         auto libId = plexData.api->GetLibraryId(plexData.libraryName);
         if (!libId || libId.value() != historyItem.libraryId)
            continue;

         auto watchTime = warp::ConvertIsoToTimePoint(historyItem.watchTime);
         if (!watchTime || (std::chrono::system_clock::now() - *watchTime) <= deleteTimeHours_)
            continue;

         if (auto itemPath = plexData.api->GetItemPath(historyItem.serverRatingKey);
             itemPath)
            return DeleteFileInfo{
               .path = warp::ReplaceMediaPath(*itemPath, plexData.mediaPath, containerPath_),
               .userName = std::string(user),
               .server = std::string(plexData.api->GetPrettyName())};
      }
      return std::nullopt;
   }

   std::optional<DeleteFileInfo> DeleteWatchedLibrary::ValidEmbyItem(const std::string_view user,
                                                                     const warp::TracearrHistoryItem& historyItem)
   {
      for (const auto& embyData : embyDatas_)
      {
         if ((historyItem.serverName != embyData.api->GetTracearrServerName()) ||
             !embyData.api->GetValid())
            continue;

         auto libId = embyData.api->GetLibraryId(embyData.libraryName);
         if (!libId || libId.value() != historyItem.libraryId)
            continue;

         auto watchTime = warp::ConvertIsoToTimePoint(historyItem.watchTime);
         if (!watchTime || (std::chrono::system_clock::now() - *watchTime) <= deleteTimeHours_)
            continue;

         if (auto itemPath = embyData.api->GetItemPath(historyItem.serverRatingKey);
             itemPath)
            return DeleteFileInfo{
               .path = warp::ReplaceMediaPath(*itemPath, embyData.mediaPath, containerPath_),
               .userName = std::string(user),
               .server = std::string(embyData.api->GetPrettyName())};
      }
      return std::nullopt;
   }

   bool DeleteWatchedLibrary::DeleteFiles(const std::vector<DeleteFileInfo>& files)
   {
      bool returnDeleted = false;
      for (const auto& file : files)
      {
         bool deletedFile = false;
         if (dryRun_)
         {
            deletedFile = true;
         }
         else
         {
            std::error_code ec;
            if (std::filesystem::remove(file.path, ec))
            {
               deletedFile = true;
            }
            else if (ec) // Only log if there's a genuine error (not just 'file not
                         // found')
            {
               serviceLogger_.LogWarning(
                  "Failed to delete {} - {}",
                  warp::GetStandoutText(file.path.generic_string()),
                  ec.message());
            }
         }

         if (deletedFile)
         {
            returnDeleted = true;
            serviceLogger_.LogInfo("{}{} watched on {} deleting {}",
                                   dryRunText_,
                                   file.userName,
                                   file.server,
                                   warp::GetTag("file", warp::GetStandoutText(file.path.generic_string())));
         }
      }

      return returnDeleted;
   }

   void DeleteWatchedLibrary::NotifyServers()
   {
      std::string syncServerNames;

      // Notify Plex Servers
      for (const auto& plex : plexDatas_)
      {
         if (!plex.api->GetValid())
            continue;

         auto libraryId = plex.api->GetLibraryId(plex.libraryName);
         if (!libraryId)
            continue;

         // Assuming Library Refresh is the intended notification
         if (!dryRun_)
            plex.api->SetLibraryScan(*libraryId);

         syncServerNames =
            warp::BuildSyncServerString(syncServerNames, plex.api->GetPrettyName(), "") + ":" + plex.libraryName;
      }

      // Notify Emby Servers
      for (const auto& emby : embyDatas_)
      {
         if (!emby.api->GetValid())
            continue;

         auto libraryId = emby.api->GetLibraryId(emby.libraryName);
         if (!libraryId)
            continue;

         if (!dryRun_)
            emby.api->SetLibraryScan(*libraryId);

         syncServerNames =
            warp::BuildSyncServerString(syncServerNames, emby.api->GetPrettyName(), "") + ":" + emby.libraryName;
      }

      if (!syncServerNames.empty())
      {
         serviceLogger_.LogInfo("{}Notified {} to refresh", dryRunText_, syncServerNames);
      }
   }

   void DeleteWatchedLibrary::ProcessDeletes(std::string_view user, const warp::TracearrHistoryItems& userHistory)
   {
      std::vector<DeleteFileInfo> deleteInfo;

      for (const auto& historyItem : userHistory.items)
      {
         if (!historyItem.watched || !historyItem.libraryId)
            continue;

         if (auto plexDeleteInfo = ValidPlexItem(user, historyItem);
             plexDeleteInfo)
         {
            deleteInfo.emplace_back(std::move(plexDeleteInfo.value()));
         }
         else if (auto embyDeleteInfo = ValidEmbyItem(user, historyItem);
                  embyDeleteInfo)
         {
            deleteInfo.emplace_back(std::move(embyDeleteInfo.value()));
         }
      }

      if (DeleteFiles(deleteInfo))
         NotifyServers();
   }
} // namespace loomis
