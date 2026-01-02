#include "service-manager.h"

#include "logger/logger.h"
#include "logger/log-utils.h"
#include "services/playlist-sync/playlist-sync-service.h"
#include "services/watch-state-sync/watch-state-sync-service.h"

#include <algorithm>
#include <format>

namespace loomis
{
   ServiceManager::ServiceManager(std::shared_ptr<ConfigReader> configReader)
      : configReader_(configReader)
      , apiManager_(std::make_shared<ApiManager>(configReader))
   {
   }

   void ServiceManager::CreateServices()
   {
      if (configReader_->GetPlaylistSyncConfig().enabled)
      {
         services_.emplace_back(std::make_unique<PlaylistSyncService>(configReader_->GetPlaylistSyncConfig(), apiManager_));
      }

      if (configReader_->GetWatchStateSyncConfig().enabled)
      {
         services_.emplace_back(std::make_unique<WatchStateSyncService>(configReader_->GetWatchStateSyncConfig(), apiManager_));
      }
   }

   void ServiceManager::Run()
   {
      CreateServices();

      if (services_.empty())
      {
         Logger::Instance().Warning("No services are enabled in the configuration.");
         return;
      }

      for (const auto& service : services_)
      {
         cronScheduler_.Add(service->GetTask());
      }

      // If the scheduler successfully started hold the run thread. If not no work to do.
      if (cronScheduler_.Start())
      {
         // Hold the main thread until shutdown is requested
         std::unique_lock<std::mutex> cvUniqueLock(runCvLock_);
         runCv_.wait(cvUniqueLock, [this] { return shutdownService_.load(); });
      }
      else
      {
         Logger::Instance().Warning("No enabled services");
      }

      Logger::Instance().Info("Run has completed");
   }

   void ServiceManager::ProcessShutdown()
   {
      Logger::Instance().Info("Shutdown request received");

      cronScheduler_.Shutdown();

      {
         std::unique_lock<std::mutex> cvUniqueLock(runCvLock_);
         shutdownService_.store(true);
         runCv_.notify_all();
      }
   }
}