#include "delete-watched-service.h"

#include "services/service-types.h"

#include <warp/log/log-utils.h>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("Delete Watched");
   }

   DeleteWatchedService::DeleteWatchedService(const DeleteWatchedConfig& config,
                                              std::shared_ptr<warp::ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_DELETE_WATCHED, apiManager, config.cron)
   {
      Init(config);
   }

   void DeleteWatchedService::Init(const DeleteWatchedConfig& config)
   {
      if (config.dryRun)
      {
         LogInfo("[DRY RUN] Enabled - No files will be physically removed.");
      }

      auto apiManager = GetApiManager();
      tracearrApi_ = apiManager->GetTracearrApi();
      if (!tracearrApi_)
      {
         LogError("Tracearr API not initialized.");
         return;
      }

      for (const auto& library : config.libraries)
      {
         auto dvrLibrary = std::make_unique<DeleteWatchedLibrary>(
            library,
            config.deleteTimeHours,
            apiManager,
            ServiceLogger(*this),
            config.dryRun);
         if (dvrLibrary->IsValid())
         {
            libraries_.emplace_back(std::move(dvrLibrary));
         }
         else
         {
            LogError("Failed to initialize DVR library for {}", warp::GetTag("path", library.containerPath.string()));
         }
      }

      validUsers_.reserve(config.tracearrUsers.size());
      if (tracearrApi_->GetValid())
      {
         for (const auto& user : config.tracearrUsers)
         {
            if (auto userInfo = tracearrApi_->GetUserByName(user.name);
                userInfo)
            {
               validUsers_.emplace_back(user.name);
            }
            else
            {
               LogWarning(
                  "{} does not have {} ... Skipping",
                  tracearrApi_->GetPrettyName(),
                  warp::GetTag("user", user.name));
            }
         }
      }
      else
      {
         // Can't check right now since the api is not valid ... add all users
         std::ranges::for_each(config.tracearrUsers, [this](const auto& user) { validUsers_.emplace_back(user.name); });
         LogInfo("{} API found but currently not valid. Adding all users.", tracearrApi_->GetPrettyName());
      }
   }

   void DeleteWatchedService::Run()
   {
      if (!tracearrApi_ || !tracearrApi_->GetValid())
      {
         LogError("Tracearr API not valid skipping run");
         return;
      }

      for (const auto& user : validUsers_)
      {
         if (!tracearrApi_->GetUserByName(user).has_value())
         {
            continue;
         }

         auto userHistory = tracearrApi_->GetWatchHistoryForUserName(user);
         if (!userHistory)
         {
            continue;
         }

         for (auto& library : libraries_)
         {
            library->ProcessDeletes(user, userHistory.value());
         }
      }
   }
} // namespace loomis
