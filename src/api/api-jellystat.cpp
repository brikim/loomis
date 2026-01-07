#include "api-jellystat.h"

#include "api/api-jellystat-json-types.h"
#include "logger/log-utils.h"

#include <glaze/glaze.hpp>

#include <format>
#include <ranges>

namespace loomis
{
   namespace
   {
      const std::string API_BASE{"/api"};
      const std::string API_GET_CONFIG{"/getconfig"};
      const std::string API_GET_USER_HISTORY{"/getUserHistory"};

      const std::string APPLICATION_JSON{"application/json"};
   }

   JellystatApi::JellystatApi(const ServerConfig& serverConfig)
      : ApiBase(serverConfig.server_name, serverConfig.tracker_url, serverConfig.tracker_api_key, "JellystatApi", utils::ANSI_CODE_JELLYSTAT)
      , client_(GetUrl())
   {
      headers_ = {
         {"x-api-token", GetApiKey()},
         {"Content-Type", APPLICATION_JSON}
      };

      constexpr time_t timeoutSec{5};
      client_.set_connection_timeout(timeoutSec);
   }

   std::string JellystatApi::BuildApiPath(std::string_view path)
   {
      return std::format("{}{}", API_BASE, path);
   }

   std::string JellystatApi::ParamsToJson(const std::list<std::pair<std::string_view, std::string_view>> params)
   {
      // Copy into a standard map so Glaze recognizes the structure
      std::map<std::string_view, std::string_view> m;
      for (const auto& [key, value] : params)
      {
         m[key] = value;
      }

      return glz::write_json(m).value_or("{}");
   }

   bool JellystatApi::GetValid()
   {
      auto res = client_.Get(BuildApiPath(API_GET_CONFIG), headers_);
      return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
   }

   std::optional<std::string> JellystatApi::GetServerReportedName()
   {
      // Jellystat api does not support server name
      return std::nullopt;
   }

   std::optional<JellystatHistoryItems> JellystatApi::GetWatchHistoryForUser(std::string_view userId)
   {
      auto payload = ParamsToJson({{ "userid", userId }});
      auto res = client_.Post(BuildApiPath(API_GET_USER_HISTORY), headers_, payload, APPLICATION_JSON);
      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      JsonHistoryResults serverResponse;
      if (auto ec = glz::read < glz::opts{.error_on_unknown_keys = false} > (serverResponse, res.value().body))
      {
         LogWarning("{} - JSON Parse Error: {}",
                    __func__, glz::format_error(ec, res.value().body));
         return std::nullopt;
      }

      JellystatHistoryItems historyItems;
      historyItems.items.reserve(serverResponse.results.size());

      for (auto& item : serverResponse.results)
      {
         historyItems.items.emplace_back(JellystateHistoryItem{
            .name = std::move(item.NowPlayingItemName),
            .id = std::move(item.NowPlayingItemId),
            .user = std::move(item.UserName),
            .dateWatched = std::move(item.ActivityDateInserted),
            .seriesName = item.SeriesName.value_or(""),
            .episodeId = item.EpisodeId.value_or("")
         });
      }
      return historyItems;
   }
}