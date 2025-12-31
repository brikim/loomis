#pragma once

#include "api/api-base.h"
#include "api/api-emby.h"
#include "api/api-jellystat.h"
#include "api/api-plex.h"
#include "api/api-tautulli.h"
#include "config-reader/config-reader.h"
#include "types.h"

#include <memory>
#include <vector>

namespace loomis
{
   class ApiManager
   {
   public:
      ApiManager(std::shared_ptr<ConfigReader> configReader);
      virtual ~ApiManager() = default;

      [[nodiscard]] ApiBase* GetApi(ApiType type, std::string_view name) const;
      [[nodiscard]] PlexApi* GetPlexApi(std::string_view name) const;
      [[nodiscard]] EmbyApi* GetEmbyApi(std::string_view name) const;
      [[nodiscard]] TautulliApi* GetTautulliApi(std::string_view name) const;
      [[nodiscard]] JellystatApi* GetJellystatApi(std::string_view name) const;

   private:
      void SetupPlexApis(const std::vector<ServerConfig>& serverConfigs);
      void SetupEmbyApis(const std::vector<ServerConfig>& serverConfigs);

      void LogServerConnectionSuccess(std::string_view serverName, ApiBase* api);
      void LogServerConnectionError(ApiBase* api);

      std::vector<std::unique_ptr<PlexApi>> plexApis_;
      std::vector<std::unique_ptr<EmbyApi>> embyApis_;
      std::vector<std::unique_ptr<TautulliApi>> tautulliApis_;
      std::vector<std::unique_ptr<JellystatApi>> jellystatApis_;

   };
}