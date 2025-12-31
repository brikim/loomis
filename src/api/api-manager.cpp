#include "api-manager.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

#include <format>
#include <ranges>

namespace loomis
{
   ApiManager::ApiManager(std::shared_ptr<ConfigReader> configReader)
   {
      SetupPlexApis(configReader->GetPlexServers());
      SetupEmbyApis(configReader->GetEmbyServers());
   }

   void ApiManager::SetupPlexApis(const std::vector<ServerConfig>& serverConfigs)
   {
      std::ranges::for_each(serverConfigs, [this](const auto& server) {
         auto& plexApi{plexApis_.emplace_back(std::make_unique<PlexApi>(server))};
         plexApi->GetValid() ? LogServerConnectionSuccess(utils::GetFormattedPlex(), plexApi.get()) : LogServerConnectionError(plexApi.get());

         if (server.tracker.valid)
         {
            auto& tautulliApi{tautulliApis_.emplace_back(std::make_unique<TautulliApi>(server))};
            tautulliApi->GetValid() ? LogServerConnectionSuccess(utils::GetFormattedTautulli(), tautulliApi.get()) : LogServerConnectionError(tautulliApi.get());
         }
      });
   }

   void ApiManager::SetupEmbyApis(const std::vector<ServerConfig>& serverConfigs)
   {
      std::ranges::for_each(serverConfigs, [this](const auto& server) {
         auto& embyApi{embyApis_.emplace_back(std::make_unique<EmbyApi>(server))};
         embyApi->GetValid() ? LogServerConnectionSuccess(utils::GetFormattedEmby(), embyApi.get()) : LogServerConnectionError(embyApi.get());

         if (server.tracker.valid)
         {
            auto& jellystatApi{jellystatApis_.emplace_back(std::make_unique<JellystatApi>(server))};
            jellystatApi->GetValid() ? LogServerConnectionSuccess(utils::GetFormattedJellystat(), jellystatApi.get()) : LogServerConnectionError(jellystatApi.get());
         }
      });
   }

   void ApiManager::LogServerConnectionSuccess(std::string_view serverName, ApiBase* api)
   {
      auto serverReportedName{api->GetServerReportedName()};
      if (serverReportedName.has_value())
      {
         Logger::Instance().Info(std::format("Connected to {}({}) successfully. Server reported {}", serverName, api->GetName(), utils::GetTag("name", serverReportedName.value())));
      }
      else
      {
         Logger::Instance().Info(std::format("Connected to {}({}) successfully.", serverName, api->GetName()));
      }
   }

   void ApiManager::LogServerConnectionError(ApiBase* api)
   {
      Logger::Instance().Warning(std::format("{}({}) server not available. Is this correct {} {}",
                                             utils::GetFormattedEmby(),
                                             api->GetName(),
                                             utils::GetTag("url", api->GetUrl()),
                                             utils::GetTag("api_key", api->GetApiKey())));
   }

   PlexApi* ApiManager::GetPlexApi(std::string_view name) const
   {
      auto iter = std::ranges::find_if(plexApis_, [name](const auto& api) {
         return api->GetName() == name;
      });
      return iter != plexApis_.end() ? iter->get() : nullptr;
   }

   EmbyApi* ApiManager::GetEmbyApi(std::string_view name) const
   {
      auto iter = std::ranges::find_if(embyApis_, [name](const auto& api) {
         return api->GetName() == name;
      });
      return iter != embyApis_.end() ? iter->get() : nullptr;
   }

   TautulliApi* ApiManager::GetTautulliApi(std::string_view name) const
   {
      auto iter = std::ranges::find_if(tautulliApis_, [name](const auto& api) {
         return api->GetName() == name;
      });
      return iter != tautulliApis_.end() ? iter->get() : nullptr;
   }

   JellystatApi* ApiManager::GetJellystatApi(std::string_view name) const
   {
      auto iter = std::ranges::find_if(jellystatApis_, [name](const auto& api) {
         return api->GetName() == name;
      });
      return iter != jellystatApis_.end() ? iter->get() : nullptr;
   }

   ApiBase* ApiManager::GetApi(ApiType type, std::string_view name) const
   {
      switch (type)
      {
         case ApiType::PLEX:
            return GetPlexApi(name);
         case ApiType::EMBY:
            return GetEmbyApi(name);
         case ApiType::TAUTULLI:
            return GetTautulliApi(name);
         case ApiType::JELLYSTAT:
            return GetJellystatApi(name);
         default:
            return nullptr;
      }
   }
}