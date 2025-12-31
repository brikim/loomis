#include "api-jellystat.h"

#include "logger/log-utils.h"

#include <json/json.hpp>

#include <format>
#include <ranges>

namespace loomis
{
   inline const std::string JELLYSTAT_API_BASE{"/api"};
   inline const std::string JELLYSTAT_API_GET_CONFIG{"/getconfig"};

   JellystatApi::JellystatApi(const ServerConfig& serverConfig)
      : ApiBase(serverConfig.name, serverConfig.tracker, "JellystatApi", utils::ANSI_CODE_JELLYSTAT)
      , client_(GetUrl())
   {
      headers_ = {
         {"x-api-token", GetApiKey()},
         {"Content-Type", "application/json"}
      };

      constexpr time_t timeoutSec{5};
      client_.set_connection_timeout(timeoutSec);
   }

   std::string JellystatApi::BuildApiPath(std::string_view path)
   {
      return std::format("{}{}", JELLYSTAT_API_BASE, path);
   }

   bool JellystatApi::GetValid()
   {
      auto res = client_.Get(BuildApiPath(JELLYSTAT_API_GET_CONFIG), headers_);
      return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
   }

   std::optional<std::string> JellystatApi::GetServerReportedName()
   {
      // Jellystat api does not support server name
      return std::nullopt;
   }
}