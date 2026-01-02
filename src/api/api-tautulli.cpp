#include "api-tautulli.h"

#include "api/json-helper.h"
#include "logger/log-utils.h"
#include "version.h"

#include <format>
#include <ranges>

namespace loomis
{
   namespace
   {
      const std::string API_BASE{"/api/v2"};

      const std::string CMD_GET_INFO{"get_tautulli_info"};
      const std::string CMD_GET_USERS("get_users");
      const std::string CMD_GET_HISTORY("get_history");

      const std::string CMD_SERVER_INFO{"get_server_info"};
      const auto PTR_SERVER_INFO_DATA = "/response/data"_json_pointer;
      const auto PTR_SERVER_INFO_PMS_NAME = "/response/data/pms_name"_json_pointer;

      constexpr std::string_view RESPONSE{"response"};
      constexpr std::string_view DATA{"data"};
      constexpr std::string_view PMS_NAME{"pms_name"};
      constexpr std::string_view USER_NAME{"username"};
      constexpr std::string_view USER("user");
      constexpr std::string_view USER_ID("user_id");
      constexpr std::string_view FRIENDLY_NAME("friendly_name");
      constexpr std::string_view INCLUDE_ACTIVITY("include_activity");
      constexpr std::string_view AFTER("after");
      constexpr std::string_view TITLE("title");
      constexpr std::string_view FULL_TITLE("full_title");
      constexpr std::string_view WATCHED_STATUS("watched_status");
      constexpr std::string_view RATING_KEY("rating_key");
      constexpr std::string_view STOPPED("stopped");
      constexpr std::string_view PERCENT_COMPLETE("percent_complete");

      const std::string USER_AGENT{std::format("Loomis/{}", LOOMIS_VERSION)};
   }

   TautulliApi::TautulliApi(const ServerConfig& serverConfig)
      : ApiBase(serverConfig.name, serverConfig.tracker, "TautulliApi", utils::ANSI_CODE_TAUTULLI)
      , client_(GetUrl())
   {
      constexpr time_t timeoutSec{5};
      client_.set_connection_timeout(timeoutSec);

      // Standardize headers
      headers_.insert({"User-Agent", USER_AGENT});
      headers_.insert({"Accept", "application/json"});
   }

   std::string TautulliApi::BuildApiPath(std::string_view cmd)
   {
      return std::format("{}?apikey={}&cmd={}", API_BASE, GetApiKey(), cmd);
   }

   const nlohmann::json* TautulliApi::GetJsonResponseData(const nlohmann::json& data)
   {
      // Find "response"
      auto respIt = data.find(RESPONSE);
      if (respIt == data.end()) return nullptr;

      // Find "data" inside "response"
      auto dataIt = respIt->find(DATA);
      if (dataIt == respIt->end()) return nullptr;

      return &(*dataIt);
   }

   bool TautulliApi::GetValid()
   {
      auto res = client_.Get(BuildApiPath(CMD_GET_INFO), headers_);
      return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
   }

   std::optional<std::string> TautulliApi::GetServerReportedName()
   {
      auto res = client_.Get(BuildApiPath(CMD_SERVER_INFO), headers_);

      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      auto jsonData = JsonSafeParse(res->body);
      if (!jsonData)
      {
         LogWarning(std::format("{} - malformed json reply received", __func__));
         return std::nullopt;
      }

      return JsonSafeGet<std::string>(*jsonData, PTR_SERVER_INFO_PMS_NAME);
   }

   std::optional<TautulliUserInfo> TautulliApi::GetUserInfo(std::string_view name)
   {
      auto res = client_.Get(BuildApiPath(CMD_GET_USERS), headers_);
      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      auto jsonData = JsonSafeParse(res->body);
      if (!jsonData)
      {
         LogWarning(std::format("{} - Malformed JSON reply received", __func__));
         return std::nullopt;
      }

      const auto* dataPtr = GetJsonResponseData(*jsonData);
      if (!dataPtr || !dataPtr->is_array())
      {
         return std::nullopt;
      }

      auto it = std::ranges::find_if(*dataPtr, [&](const auto& item) {
         return JsonSafeGet<std::string>(item, USER_NAME).value_or("") == name;
      });

      if (it == dataPtr->end())
      {
         return std::nullopt;
      }

      TautulliUserInfo info;
      auto friendlyName = JsonSafeGet<std::string>(*it, FRIENDLY_NAME);

      info.id = JsonSafeGet<int32_t>(*it, USER_ID).value_or(0);
      info.friendlyNameValid = friendlyName.has_value();
      info.friendlyName = friendlyName.value_or("");

      return info;
   }

   std::optional<TautulliHistoryItems> TautulliApi::GetWatchHistoryForUser(std::string_view user, std::string_view dateForHistory)
   {
      auto apiPath = BuildApiPath(CMD_GET_HISTORY);
      AddApiParam(apiPath, {
          {INCLUDE_ACTIVITY, "0"},
          {USER, user},
          {AFTER, dateForHistory}
      });

      auto res = client_.Get(apiPath, headers_);
      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      auto jsonData = JsonSafeParse(res->body);
      if (!jsonData) return std::nullopt;

      const auto* dataPtr = GetJsonResponseData(*jsonData);

      // Safety check: Ensure the pointer is valid and the 'data' field is an array
      if (!dataPtr || !dataPtr->contains(DATA) || !(*dataPtr)[DATA].is_array())
      {
         return std::nullopt;
      }

      const auto& historyArray = (*dataPtr)[DATA];
      if (historyArray.empty()) return std::nullopt;

      // 4. Map to Structs
      TautulliHistoryItems returnHistory;
      returnHistory.items.reserve(historyArray.size());

      for (const auto& item : historyArray)
      {
         // Use C++20 designated initializers for maximum clarity
         returnHistory.items.emplace_back(TautulliHistoryItem{
             .name = JsonSafeGet<std::string>(item, TITLE).value_or(""),
             .fullName = JsonSafeGet<std::string>(item, FULL_TITLE).value_or(""),
             .id = JsonSafeGet<int32_t>(item, RATING_KEY).value_or(0),
             .watched = (JsonSafeGet<int32_t>(item, WATCHED_STATUS).value_or(0) == 1),
             .timeWatchedEpoch = JsonSafeGet<int64_t>(item, STOPPED).value_or(0),
             .playbackPercentage = JsonSafeGet<int32_t>(item, PERCENT_COMPLETE).value_or(0)
         });
      }

      return returnHistory;
   }
}