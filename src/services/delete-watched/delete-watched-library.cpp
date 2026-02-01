#include "delete-watched-library.h"

#include "services/service-utils.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-manager.h>
#include <warp/api/api-plex.h>
#include <warp/log/log.h>
#include <warp/log/log-utils.h>
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
      , historyDays_(std::max((deleteTimeHours / 24) + 1, 1))
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
         auto* api = apiManager_->GetPlexApi(plexConfig.server);
         if (!api)
         {
            serviceLogger_.LogWarning("No {} found with {}",
                                      warp::GetFormattedPlex(),
                                      warp::GetTag("server_name", plexConfig.server));
         }

         auto* trackerApi = apiManager_->GetTautulliApi(plexConfig.server);
         if (!trackerApi)
         {
            serviceLogger_.LogWarning("No {} found with {}",
                                      warp::GetFormattedTautulli(),
                                      warp::GetTag("server_name", plexConfig.server));
         }

         // One of the required api's was not found
         if (!api || !trackerApi) continue;

         bool libraryValid = true;
         if (api->GetValid())
         {
            if (auto libId = api->GetLibraryId(plexConfig.library);
                !libId)
            {
               serviceLogger_.LogWarning("{} does not have {} ... Skipping",
                                         api->GetPrettyName(),
                                         warp::GetTag("library", plexConfig.library));
               libraryValid = false;
            }
         }

         std::vector<std::string> validUsers;
         validUsers.reserve(plexConfig.users.size());
         if (trackerApi->GetValid())
         {
            for (const auto& user : plexConfig.users)
            {
               if (auto userInfo = trackerApi->GetUserInfo(user.name);
                   userInfo)
               {
                  validUsers.emplace_back(user.name);
               }
               else
               {
                  serviceLogger_.LogWarning("{} does not have {} ... Skipping",
                                            api->GetPrettyName(),
                                            warp::GetTag("user", user.name));
               }
            }
         }
         else
         {
            // Can't check right now since the api is not valid ... add all users
            std::ranges::for_each(plexConfig.users, [&validUsers](const auto& user) {
               validUsers.emplace_back(user.name);
            });
         }

         if (libraryValid && !validUsers.empty())
         {
            plexDatas_.emplace_back(DeleteWatchedPlexData{
               .api = api,
               .trackerApi = trackerApi,
               .libraryName = plexConfig.library,
               .mediaPath = plexConfig.mediaPath,
               .users = std::move(validUsers)
            });
         }
      }

      for (const auto& embyConfig : config.emby)
      {
         auto* api = apiManager_->GetEmbyApi(embyConfig.server);
         if (!api)
         {
            serviceLogger_.LogWarning("No {} found with {}",
                                      warp::GetFormattedEmby(),
                                      warp::GetTag("server_name", embyConfig.server));
         }

         auto* trackerApi = apiManager_->GetJellystatApi(embyConfig.server);
         if (!trackerApi)
         {
            serviceLogger_.LogWarning("No {} found with {}",
                                      warp::GetFormattedJellystat(),
                                      warp::GetTag("server_name", embyConfig.server));
         }

         // One of the required api's was not found
         if (!api || !trackerApi) continue;

         bool libraryValid = true;
         std::vector<std::string> validUsers;
         validUsers.reserve(embyConfig.users.size());
         if (api->GetValid())
         {
            if (auto libId = api->GetLibraryId(embyConfig.library);
                !libId)
            {
               libraryValid = false;
               serviceLogger_.LogWarning("{} does not have {} ... Skipping",
                                         api->GetPrettyName(),
                                         warp::GetTag("library", embyConfig.library));
            }

            for (const auto& user : embyConfig.users)
            {
               if (auto userData = api->GetUser(user.name);
                   userData)
               {
                  validUsers.emplace_back(user.name);
               }
               else
               {
                  serviceLogger_.LogWarning("{} does not have {} ... Skipping",
                                            api->GetPrettyName(),
                                            warp::GetTag("user", user.name));
               }
            }
         }
         else
         {
            // Can't check right now since the api is not valid ... add all users
            std::ranges::for_each(embyConfig.users, [&validUsers](const auto& user) {
               validUsers.emplace_back(user.name);
            });
         }

         if (libraryValid && !validUsers.empty())
         {
            embyDatas_.emplace_back(DeleteWatchedEmbyData{
               .api = api,
               .trackerApi = trackerApi,
               .libraryName = embyConfig.library,
               .mediaPath = embyConfig.mediaPath,
               .users = std::move(validUsers)
            });
         }
      }

      valid_ = (plexDatas_.size() + embyDatas_.size()) > 0;
   }

   bool DeleteWatchedLibrary::IsValid() const
   {
      return valid_;
   }

   std::vector<DeleteFileInfo> DeleteWatchedLibrary::FindPlexWatched(const DeleteWatchedPlexData& data,
                                                                     const std::string& dataTimeForHistory,
                                                                     int64_t epochDateTimeForHistory)
   {
      std::vector<DeleteFileInfo> returnDeletes;

      auto libraryId = data.api->GetLibraryId(data.libraryName);
      if (!libraryId) return {};

      for (const auto& user : data.users)
      {
         auto userHistory = data.trackerApi->GetWatchHistoryForUserAndLibrary(user,
                                                                              *libraryId,
                                                                              dataTimeForHistory,
                                                                              epochDateTimeForHistory);
         if (!userHistory) continue;

         for (auto& item : userHistory->items)
         {
            if (!item.watched) continue;

            auto itemPath = data.api->GetItemPath(item.id);

            if (!itemPath) continue;

            std::chrono::system_clock::time_point lastWatched{std::chrono::seconds{item.timeWatchedEpoch}};

            if ((std::chrono::system_clock::now() - lastWatched) <= deleteTimeHours_) continue;

            returnDeletes.emplace_back(DeleteFileInfo{
               .path = ReplaceMediaPath(*itemPath, data.mediaPath, containerPath_),
               .userName = user,
               .server = data.api->GetPrettyName()
            });
         }
      }

      return returnDeletes;
   }

   std::vector<DeleteFileInfo> DeleteWatchedLibrary::FindEmbyWatched(const DeleteWatchedEmbyData& data)
   {
      auto libraryId = data.api->GetLibraryId(data.libraryName);
      if (!libraryId) return {};

      auto libraryHistory = data.trackerApi->GetWatchHistoryForLibrary(*libraryId);
      if (!libraryHistory) return {};

      std::vector<DeleteFileInfo> returnDeletes;
      for (const auto& item : libraryHistory->items)
      {
         auto userIter = std::ranges::find_if(data.users, [&item](const auto& user) {
            return item.user == user;
         });

         // User is not in the list to be considered watch ... continue
         if (userIter == data.users.end()) continue;

         auto userData = data.api->GetUser(*userIter);
         if (!userData) continue;

         auto playState = data.api->GetPlayState(userData->id, item.episodeId ? *item.episodeId : item.id);
         if (!playState || !playState->watched) continue;

         // Check if the show if the amount of time has elapsed before delete
         auto watchTime = warp::ConvertIsoToTimePoint(item.watchTime);
         if (!watchTime || (std::chrono::system_clock::now() - *watchTime) <= deleteTimeHours_) continue;

         returnDeletes.emplace_back(DeleteFileInfo{
               .path = ReplaceMediaPath(playState->path, data.mediaPath, containerPath_),
               .userName = *userIter,
               .server = data.api->GetPrettyName()
            });
      }

      return returnDeletes;
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
            else if (ec) // Only log if there's a genuine error (not just 'file not found')
            {
               serviceLogger_.LogWarning("Failed to delete {} - {}",
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
         if (!plex.api->GetValid()) continue;

         auto libraryId = plex.api->GetLibraryId(plex.libraryName);
         if (!libraryId) continue;

         // Assuming Library Refresh is the intended notification
         if (!dryRun_) plex.api->SetLibraryScan(*libraryId);

         syncServerNames = warp::BuildSyncServerString(syncServerNames, plex.api->GetPrettyName(), "") + ":" + plex.libraryName;
      }

      // Notify Emby Servers
      for (const auto& emby : embyDatas_)
      {
         if (!emby.api->GetValid()) continue;

         auto libraryId = emby.api->GetLibraryId(emby.libraryName);
         if (!libraryId) continue;

         if (!dryRun_) emby.api->SetLibraryScan(*libraryId);

         syncServerNames = warp::BuildSyncServerString(syncServerNames, emby.api->GetPrettyName(), "") + ":" + emby.libraryName;
      }

      if (!syncServerNames.empty())
      {
         serviceLogger_.LogInfo("{}Notified {} to refresh",
                                dryRunText_,
                                syncServerNames);
      }
   }

   void DeleteWatchedLibrary::Run()
   {
      std::vector<DeleteFileInfo> deleteInfo;

      auto dataTimeForHistoryPlex = GetDatetimeForHistoryPlex(historyDays_);
      auto plexEpochHistoryTime = GetEpochTimeForPlexHistory(historyDays_);
      for (const auto& plexData : plexDatas_)
      {
         auto plexDeleteInfo = FindPlexWatched(plexData, dataTimeForHistoryPlex, plexEpochHistoryTime);
         deleteInfo.reserve(deleteInfo.size() + plexDeleteInfo.size());
         deleteInfo.insert(deleteInfo.end(), std::make_move_iterator(plexDeleteInfo.begin()), std::make_move_iterator(plexDeleteInfo.end()));
      }

      for (const auto& embyData : embyDatas_)
      {
         auto embyDeleteInfo = FindEmbyWatched(embyData);
         deleteInfo.reserve(deleteInfo.size() + embyDeleteInfo.size());
         deleteInfo.insert(deleteInfo.end(), std::make_move_iterator(embyDeleteInfo.begin()), std::make_move_iterator(embyDeleteInfo.end()));
      }

      if (DeleteFiles(deleteInfo)) NotifyServers();
   }
}