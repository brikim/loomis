#include "service-manager.h"

#include "services/dvr-maintainer/dvr-maintainer-service.h"
#include "services/folder-cleanup/folder-cleanup-service.h"
#include "services/playlist-sync/playlist-sync-service.h"
#include "services/watch-state-sync/watch-state-sync-service.h"
#include "version.h"

#include <warp/log/log.h>
#include <warp/log/log-utils.h>

#include <algorithm>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("Loomis");
   };

   ServiceManager::ServiceManager(std::shared_ptr<ConfigReader> configReader)
      : configReader_(configReader)
   {
      std::vector<warp::ServerConfig> plexConfigs;
      for (const auto& plexServer : configReader_->GetPlexServers())
      {
         plexConfigs.emplace_back(warp::ServerConfig{
                                  .server_name = plexServer.server_name,
                                  .url = plexServer.url,
                                  .api_key = plexServer.api_key,
                                  .tracker_url = plexServer.tracker_url,
                                  .tracker_api_key = plexServer.tracker_api_key,
                                  .media_path = plexServer.media_path});
      }

      std::vector<warp::ServerConfig> embyConfigs;
      for (const auto& embyServer : configReader_->GetEmbyServers())
      {
         embyConfigs.emplace_back(warp::ServerConfig{
                                  .server_name = embyServer.server_name,
                                  .url = embyServer.url,
                                  .api_key = embyServer.api_key,
                                  .tracker_url = embyServer.tracker_url,
                                  .tracker_api_key = embyServer.tracker_api_key,
                                  .media_path = embyServer.media_path});
      }
      apiManager_ = std::make_shared<warp::ApiManager>(SERVICE_NAME,
                                                       LOOMIS_VERSION,
                                                       plexConfigs,
                                                       embyConfigs);

      // Loomis requires the extra caching provided by the api manager
      apiManager_->EnableExtraCaching();
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

      if (configReader_->GetFolderCleanupConfig().enabled)
      {
         services_.emplace_back(std::make_unique<FolderCleanupService>(configReader_->GetFolderCleanupConfig(), apiManager_));
      }

      if (configReader_->GetDvrMaintainerConfig().enabled)
      {
         services_.emplace_back(std::make_unique<DvrMaintainerService>(configReader_->GetDvrMaintainerConfig(), apiManager_));
      }
   }

   void ServiceManager::Run()
   {
      CreateServices();

      if (services_.empty())
      {
         warp::log::Critical("No services are enabled in the configuration.");
         return;
      }

      // Add any needed api tasks
      std::vector<warp::Task> apiTasks;
      apiManager_->GetTasks(apiTasks);
      for (const auto& apiTask : apiTasks)
      {
         cronScheduler_.Add(apiTask);
      }
      apiTasks.clear();

      // Services are required to have tasks so add
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
         warp::log::Critical("No enabled services");
      }

      warp::log::Info("Run has completed");
   }

   void ServiceManager::ProcessShutdown()
   {
      warp::log::Info("Shutdown request received");

      cronScheduler_.Shutdown();

      {
         std::unique_lock<std::mutex> cvUniqueLock(runCvLock_);
         shutdownService_.store(true);
         runCv_.notify_all();
      }
   }
}