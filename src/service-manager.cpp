#include "service-manager.h"

#include "services/delete-watched/delete-watched-service.h"
#include "services/dvr-maintainer/dvr-maintainer-service.h"
#include "services/emby-tidy/emby-tidy-service.h"
#include "services/filesystem-cleanup/filesystem-cleanup-service.h"
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
      warp::ApiManagerConfig apiManagerConfig;

      std::vector<warp::ServerConfig> plexConfigs;
      for (const auto& plexServer : configReader_->GetPlexServers())
      {
         apiManagerConfig.plexConfig.servers.emplace_back(warp::ServerConfig{
            .serverName = plexServer.server_name,
            .url = plexServer.url,
            .apiKey = plexServer.api_key,
            .trackerUrl = plexServer.tracker_url,
            .trackerApiKey = plexServer.tracker_api_key,
            .mediaPath = plexServer.media_path});
      }
      apiManagerConfig.plexConfig.options.enableCacheCollection = true;
      apiManagerConfig.plexConfig.options.enableCachePaths = true;
      apiManagerConfig.plexConfig.options.enableUserTokens = true;

      std::vector<warp::ServerConfig> embyConfigs;
      for (const auto& embyServer : configReader_->GetEmbyServers())
      {
         apiManagerConfig.embyConfig.servers.emplace_back(warp::ServerConfig{
            .serverName = embyServer.server_name,
            .url = embyServer.url,
            .apiKey = embyServer.api_key,
            .trackerUrl = embyServer.tracker_url,
            .trackerApiKey = embyServer.tracker_api_key,
            .mediaPath = embyServer.media_path});
      }
      apiManagerConfig.embyConfig.options.enableCachePaths = true;

      apiManager_ = std::make_shared<warp::ApiManager>(SERVICE_NAME,
                                                       LOOMIS_VERSION,
                                                       apiManagerConfig);
   }

   void ServiceManager::CreateServices()
   {
      if (configReader_->GetPlaylistSyncConfig().enabled)
         services_.emplace_back(std::make_unique<PlaylistSyncService>(configReader_->GetPlaylistSyncConfig(), apiManager_));

      if (configReader_->GetWatchStateSyncConfig().enabled)
         services_.emplace_back(std::make_unique<WatchStateSyncService>(configReader_->GetWatchStateSyncConfig(), apiManager_));

      if (configReader_->GetFilesystemCleanupConfig().enabled)
         services_.emplace_back(std::make_unique<FilesystemCleanupService>(configReader_->GetFilesystemCleanupConfig(), apiManager_));

      if (configReader_->GetDvrMaintainerConfig().enabled)
         services_.emplace_back(std::make_unique<DvrMaintainerService>(configReader_->GetDvrMaintainerConfig(), apiManager_));

      if (configReader_->GetDeleteWatchedConfig().enabled)
         services_.emplace_back(std::make_unique<DeleteWatchedService>(configReader_->GetDeleteWatchedConfig(), apiManager_));

      if (configReader_->GetEmbyTidyConfig().enabled)
         services_.emplace_back(std::make_unique<EmbyTidyService>(configReader_->GetEmbyTidyConfig(), apiManager_));
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
         std::mutex m;
         std::unique_lock lk(m);
         std::condition_variable_any().wait(lk, stopSource_.get_token(), [] { return false; });
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

      apiManager_->Shutdown();
      cronScheduler_.Shutdown();

      stopSource_.request_stop();
   }
}