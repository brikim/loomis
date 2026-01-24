#include "api-manager.h"

#include <warp/log.h>
#include <warp/log-utils.h>

#include <format>

namespace loomis
{
   ApiManager::ApiManager(std::shared_ptr<ConfigReader> configReader)
   {
      SetupPlexApis(configReader->GetPlexServers());
      SetupEmbyApis(configReader->GetEmbyServers());
   }

   void ApiManager::SetupPlexApis(const std::vector<ServerConfig>& serverConfigs)
   {
      for (const auto& server : serverConfigs)
      {
         InitializeApi<PlexApi>(plexApis_, server, warp::GetFormattedPlex());

         if (!server.tracker_url.empty())
         {
            InitializeApi<TautulliApi>(tautulliApis_, server, warp::GetFormattedTautulli());
         }
      }
   }

   void ApiManager::SetupEmbyApis(const std::vector<ServerConfig>& serverConfigs)
   {
      for (const auto& server : serverConfigs)
      {
         InitializeApi<EmbyApi>(embyApis_, server, warp::GetFormattedEmby());

         if (!server.tracker_url.empty())
         {
            InitializeApi<JellystatApi>(jellystatApis_, server, warp::GetFormattedJellystat());
         }
      }
   }

   void ApiManager::AddTasks(CronScheduler& cronScheduler)
   {
      InitializeTasks(cronScheduler, plexApis_);
      InitializeTasks(cronScheduler, tautulliApis_);
      InitializeTasks(cronScheduler, embyApis_);
      InitializeTasks(cronScheduler, jellystatApis_);
   }

   void ApiManager::LogServerConnectionSuccess(std::string_view serverName, ApiBase* api)
   {
      auto reported = api->GetServerReportedName();
      warp::log::Info("Connected to {}({}) successfully.{}",
                              serverName, api->GetName(),
                              reported ? std::format(" Server reported {}", warp::GetTag("name", *reported)) : "");
   }

   void ApiManager::LogServerConnectionError(std::string_view serverName, ApiBase* api)
   {
      warp::log::Warning("{}({}) server not available. Is this correct? {} {}",
                                 serverName, api->GetName(),
                                 warp::GetTag("url", api->GetUrl()),
                                 warp::GetTag("api_key", api->GetApiKey()));
   }

   PlexApi* ApiManager::GetPlexApi(std::string_view name) const
   {
      return FindApi(plexApis_, name);
   }

   EmbyApi* ApiManager::GetEmbyApi(std::string_view name) const
   {
      return FindApi(embyApis_, name);
   }

   TautulliApi* ApiManager::GetTautulliApi(std::string_view name) const
   {
      return FindApi(tautulliApis_, name);
   }

   JellystatApi* ApiManager::GetJellystatApi(std::string_view name) const
   {
      return FindApi(jellystatApis_, name);
   }

   ApiBase* ApiManager::GetApi(warp::ApiType type, std::string_view name) const
   {
      switch (type)
      {
         case warp::ApiType::PLEX:      return FindApi(plexApis_, name);
         case warp::ApiType::EMBY:      return FindApi(embyApis_, name);
         case warp::ApiType::TAUTULLI:  return FindApi(tautulliApis_, name);
         case warp::ApiType::JELLYSTAT: return FindApi(jellystatApis_, name);
         default:                 return nullptr;
      }
   }
}