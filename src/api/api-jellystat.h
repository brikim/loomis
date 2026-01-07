#pragma once

#include "api/api-base.h"
#include "api/api-jellystat-types.h"
#include "config-reader/config-reader-types.h"

#include <httplib.h>

#include <list>
#include <optional>
#include <string>

namespace loomis
{
   class JellystatApi : public ApiBase
   {
   public:
      JellystatApi(const ServerConfig& serverConfig);
      virtual ~JellystatApi() = default;

      // Returns true if the server is reachable and the API key is valid
      [[nodiscard]] bool GetValid() override;
      [[nodiscard]] std::optional<std::string> GetServerReportedName() override;

      [[nodiscard]] std::optional<JellystatHistoryItems> GetWatchHistoryForUser(std::string_view userId);

   private:
      std::string BuildApiPath(std::string_view path);
      std::string ParamsToJson(const std::list<std::pair<std::string_view, std::string_view>> params);

      httplib::Client client_;
      httplib::Headers headers_;
   };
}