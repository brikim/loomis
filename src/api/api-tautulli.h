#pragma once

#include "api/api-base.h"
#include "api/api-tautulli-types.h"
#include "config-reader/config-reader-types.h"

#include <httplib/httplib.h>
#include <json/json.hpp>

#include <cstdint>
#include <list>
#include <optional>
#include <string>

namespace loomis
{
   class TautulliApi : public ApiBase
   {
   public:
      TautulliApi(const ServerConfig& serverConfig);
      virtual ~TautulliApi() = default;

      // Returns true if the server is reachable and the API key is valid
      [[nodiscard]] bool GetValid() override;
      [[nodiscard]] std::optional<std::string> GetServerReportedName() override;

      [[nodiscard]] std::optional<TautulliUserInfo> GetUserInfo(std::string_view name);

      [[nodiscard]] std::optional<TautulliHistoryItems> GetWatchHistoryForUser(std::string_view user, std::string_view dateForHistory);

   private:
      std::string BuildApiPath(std::string_view cmd);
      const nlohmann::json* GetJsonResponseData(const nlohmann::json& data);

      httplib::Client client_;
      httplib::Headers headers_;
   };
}