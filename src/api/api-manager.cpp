#include "api-manager.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

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
         InitializeApi<PlexApi>(plexApis_, server, utils::GetFormattedPlex());

         if (server.tracker.valid)
         {
            InitializeApi<TautulliApi>(tautulliApis_, server, utils::GetFormattedTautulli());
         }
      }
   }

   void ApiManager::SetupEmbyApis(const std::vector<ServerConfig>& serverConfigs)
   {
      for (const auto& server : serverConfigs)
      {
         InitializeApi<EmbyApi>(embyApis_, server, utils::GetFormattedEmby());

         if (server.tracker.valid)
         {
            InitializeApi<JellystatApi>(jellystatApis_, server, utils::GetFormattedJellystat());
         }
      }
   }

   void ApiManager::LogServerConnectionSuccess(std::string_view serverName, ApiBase* api)
   {
      auto reported = api->GetServerReportedName();
      Logger::Instance().Info(std::format("Connected to {}({}) successfully.{}",
                                          serverName, api->GetName(),
                                          reported ? std::format(" Server reported {}", utils::GetTag("name", *reported)) : ""));
   }

   void ApiManager::LogServerConnectionError(std::string_view serverName, ApiBase* api)
   {
      Logger::Instance().Warning(std::format("{}({}) server not available. Is this correct? {} {}",
                                             serverName, api->GetName(),
                                             utils::GetTag("url", api->GetUrl()),
                                             utils::GetTag("api_key", api->GetApiKey())));
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

   ApiBase* ApiManager::GetApi(ApiType type, std::string_view name) const
   {
      switch (type)
      {
         case ApiType::PLEX:      return FindApi(plexApis_, name);
         case ApiType::EMBY:      return FindApi(embyApis_, name);
         case ApiType::TAUTULLI:  return FindApi(tautulliApis_, name);
         case ApiType::JELLYSTAT: return FindApi(jellystatApis_, name);
         default:                 return nullptr;
      }
   }
}