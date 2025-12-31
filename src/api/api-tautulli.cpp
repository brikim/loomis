#include "api-tautulli.h"

#include "api/json-helper.h"
#include "logger/log-utils.h"

#include <json/json.hpp>

#include <format>
#include <ranges>

namespace loomis
{
   namespace
   {
      const std::string API_BASE{"/api/v2"};

      const std::string CMD_GET_INFO{"get_tautulli_info"};

      const std::string CMD_SERVER_INFO{"get_server_info"};
      const auto PTR_SERVER_INFO_PMS_NAME = "/response/data/pms_name"_json_pointer;

      constexpr std::string_view ELEM_RESPONSE{"response"};
      constexpr std::string_view ELEM_DATA{"data"};
      constexpr std::string_view ELEM_PMS_NAME{"pms_name"};
   }

   TautulliApi::TautulliApi(const ServerConfig& serverConfig)
      : ApiBase(serverConfig.name, serverConfig.tracker, "TautulliApi", utils::ANSI_CODE_TAUTULLI)
      , client_(GetUrl())
   {
      constexpr time_t timeoutSec{5};
      client_.set_connection_timeout(timeoutSec);
   }

   std::string TautulliApi::BuildApiPath(std::string_view cmd)
   {
      return std::format("{}?apikey={}&cmd={}", API_BASE, GetApiKey(), cmd);
   }

   bool TautulliApi::GetValid()
   {
      auto res = client_.Get(BuildApiPath(CMD_GET_INFO), headers_);
      return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
   }

   std::optional<std::string> TautulliApi::GetServerReportedName()
   {
      if (auto res = client_.Get(BuildApiPath(CMD_SERVER_INFO), headers_);
          IsHttpSuccess("GetServerReportedName", res))
      {
         if (auto jsonData = JsonSafeParse(res.value().body);
             jsonData.has_value())
         {
            if (auto pmsName = JsonSafeGet<std::string>(jsonData.value(), PTR_SERVER_INFO_PMS_NAME);
                pmsName.has_value())
            {
               return pmsName.value();
            }
         }
         else
         {
            LogWarning("GetServerReportedName malformed json reply received");
         }
      }
      return std::nullopt;
   }
}