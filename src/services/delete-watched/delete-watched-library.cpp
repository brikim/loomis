#include "delete-watched-library.h"

#include "services/service-utils.h"

#include <warp/log/log.h>
#include <warp/log/log-utils.h>
#include <warp/utils.h>

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

         if (api && trackerApi)
         {
            if (api->GetValid())
            {
               auto libId = api->GetLibraryId(plexConfig.library);
               if (!libId)
               {
                  serviceLogger_.LogWarning("{} does not have {} ... Skipping",
                                            api->GetPrettyName(),
                                            warp::GetTag("library", plexConfig.library));
               }
            }

            std::vector<std::string> users;
            users.reserve(plexConfig.users.size());
            std::ranges::for_each(plexConfig.users, [&users](const auto& user) {
               users.emplace_back(user.name);
            });

            bool usersValid = true;
            if (trackerApi->GetValid())
            {
               for (const auto& user : users)
               {
                  auto userInfo = trackerApi->GetUserInfo(user);
                  if (!userInfo)
                  {
                     serviceLogger_.LogWarning("{} does not have {} ... Skipping",
                                               api->GetPrettyName(),
                                               warp::GetTag("user", user));
                     usersValid = false;
                  }
               }
            }

            plexDatas_.emplace_back(DeleteWatchedPlexData{
               .api = api,
               .trackerApi = trackerApi,
               .libraryName = plexConfig.library,
               .users = std::move(users)
            });
         }
      }

      for (const auto& embyConfig : config.emby)
      {
         auto api = apiManager_->GetEmbyApi(embyConfig.server);
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

         if (api && trackerApi)
         {
            std::vector<std::string> users;
            users.reserve(embyConfig.users.size());
            std::ranges::for_each(embyConfig.users, [&users](const auto& user) {
               users.emplace_back(user.name);
            });

            if (api->GetValid())
            {
               auto libId = api->GetLibraryId(embyConfig.library);
               if (!libId)
               {
                  serviceLogger_.LogWarning("{} does not have {} ... Skipping",
                                            api->GetPrettyName(),
                                            warp::GetTag("library", embyConfig.library));
               }

               for (const auto& user : users)
               {
                  auto userData = api->GetUser(user);
                  if (!userData)
                  {
                     serviceLogger_.LogWarning("{} does not have {} ... Skipping",
                                               api->GetPrettyName(),
                                               warp::GetTag("user", user));
                  }
               }
            }

            embyDatas_.emplace_back(DeleteWatchedEmbyData{
               .api = api,
               .trackerApi = trackerApi,
               .libraryName = embyConfig.library,
               .users = std::move(users)
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
               .id = item.id,
               .path = ReplaceMediaPath(*itemPath, data.api->GetMediaPath(), containerPath_),
               .userName = user,
               .server = data.api->GetPrettyName()
            });
         }
      }

      return returnDeletes;
   }

   std::vector<DeleteFileInfo> DeleteWatchedLibrary::FindEmbyWatched(const DeleteWatchedEmbyData& data,
                                                                     const std::string& dataTimeForHistory,
                                                                     int64_t epochDateTimeForHistory)
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

         if (userIter == data.users.end()) continue;

         auto userData = data.api->GetUser(*userIter);
         if (!userData) continue;

         auto playState = data.api->GetPlayState(userData->id, item.episodeId ? *item.episodeId : item.id);
         if (!playState || !playState->watched) continue;

         auto watchTime = warp::ConvertIsoToTimePoint(item.watchTime);
         if (!watchTime || (std::chrono::system_clock::now() - *watchTime) <= deleteTimeHours_) continue;

         returnDeletes.emplace_back(DeleteFileInfo{
               .id = item.id,
               .path = ReplaceMediaPath(playState->path, data.api->GetMediaPath(), containerPath_),
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
                                         warp::GetStandoutText(file.path.string()),
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
                                   warp::GetTag("file", warp::GetStandoutText(file.path.string())));
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
         auto embyDeleteInfo = FindEmbyWatched(embyData, dataTimeForHistoryPlex, plexEpochHistoryTime);
         deleteInfo.reserve(deleteInfo.size() + embyDeleteInfo.size());
         deleteInfo.insert(deleteInfo.end(), std::make_move_iterator(embyDeleteInfo.begin()), std::make_move_iterator(embyDeleteInfo.end()));
      }

      if (DeleteFiles(deleteInfo)) NotifyServers();
   }
}