#include "api-tautulli.h"

#include "api/api-tautulli-json-types.h"
#include "logger/log-utils.h"
#include "version.h"

#include <glaze/glaze.hpp>

#include <format>
#include <ranges>

namespace loomis
{
   namespace
   {
      const std::string API_BASE{"/api/v2"};
      const std::string API_TOKEN_NAME{"apikey"};
      const std::string API_COMMAND{"cmd"};

      const std::string CMD_GET_SERVER_FRIENDLY_NAME("get_server_friendly_name");
      const std::string CMD_GET_SETTINGS{"get_settings"};
      const std::string CMD_GET_USERS("get_users");
      const std::string CMD_GET_HISTORY("get_history");
      const std::string CMD_SERVER_INFO{"get_server_info"};

      constexpr std::string_view USER("user");
      constexpr std::string_view INCLUDE_ACTIVITY("include_activity");
      constexpr std::string_view AFTER("after");
      constexpr std::string_view SEARCH("search");

      const std::string USER_AGENT{std::format("Loomis/{}", LOOMIS_VERSION)};
   }

   TautulliApi::TautulliApi(const ServerConfig& serverConfig)
      : ApiBase(serverConfig.server_name, serverConfig.tracker_url, serverConfig.tracker_api_key, "TautulliApi", utils::ANSI_CODE_TAUTULLI)
      , client_(GetUrl())
   {
      constexpr time_t timeoutSec{5};
      client_.set_connection_timeout(timeoutSec);

      // Standardize headers
      headers_.insert({"User-Agent", USER_AGENT});
      headers_.insert({"Accept", "application/json"});
   }

   std::optional<std::vector<Task>> TautulliApi::GetTaskList()
   {
      std::vector<Task> tasks;

      auto& fullUpdate = tasks.emplace_back();
      fullUpdate.name = std::format("TautulliApi({}) - Settings Update", GetName());
      fullUpdate.cronExpression = "0 50 3 * * *";
      fullUpdate.func = [this]() {this->RunSettingsUpdate(); };

      return tasks;
   }

   std::string_view TautulliApi::GetApiBase() const
   {
      return API_BASE;
   }

   std::string_view TautulliApi::GetApiTokenName() const
   {
      return API_TOKEN_NAME;
   }

   std::pair<std::string_view, std::string_view> TautulliApi::GetCmdParam(std::string_view cmd) const
   {
      return {API_COMMAND, cmd};
   }

   bool TautulliApi::GetValid()
   {
      auto apiPath = BuildApiParamsPath("", {GetCmdParam(CMD_GET_SERVER_FRIENDLY_NAME)});
      auto res = client_.Get(apiPath, headers_);
      return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
   }

   std::optional<std::string> TautulliApi::GetServerReportedName()
   {
      auto res = client_.Get(BuildApiParamsPath("", {GetCmdParam(CMD_SERVER_INFO)}), headers_);

      if (!IsHttpSuccess(__func__, res))
      {
         return std::nullopt;
      }

      JsonTautulliResponse<JsonTautulliServerInfo> serverResponse;
      if (auto ec = glz::read < glz::opts{.error_on_unknown_keys = false} > (serverResponse, res.value().body))
      {
         LogWarning("{} - JSON Parse Error: {}",
                    __func__, glz::format_error(ec, res.value().body));
         return std::nullopt;
      }

      if (serverResponse.response.data.pms_name.empty())
      {
         return std::nullopt;
      }
      return std::move(serverResponse.response.data.pms_name);
   }

   std::optional<TautulliUserInfo> TautulliApi::GetUserInfo(std::string_view name)
   {
      auto res = client_.Get(BuildApiParamsPath("", {GetCmdParam(CMD_GET_USERS)}), headers_);
      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      JsonTautulliResponse<std::vector<JsonUserInfo>> serverResponse;
      if (auto ec = glz::read < glz::opts{.error_on_unknown_keys = false} > (serverResponse, res.value().body))
      {
         LogWarning("{} - JSON Parse Error: {}",
                    __func__, glz::format_error(ec, res.value().body));
         return std::nullopt;
      }

      auto it = std::ranges::find_if(serverResponse.response.data, [&](const auto& item) {
         return item.username == name;
      });

      if (it == serverResponse.response.data.end())
      {
         return std::nullopt;
      }

      return TautulliUserInfo{it->user_id, std::move(it->friendly_name)};
   }

   bool TautulliApi::ReadMonitoringData()
   {
      auto apiPath = BuildApiParamsPath("", {
         GetCmdParam(CMD_GET_SETTINGS),
          {"key", "Monitoring"},
      });

      auto res = client_.Get(apiPath, headers_);
      if (!IsHttpSuccess(__func__, res, false)) return false;

      JsonTautulliResponse<JsonTautulliMonitorInfo> serverResponse;
      if (auto ec = glz::read < glz::opts{.error_on_unknown_keys = false} > (serverResponse, res.value().body))
      {
         LogWarning("{} - JSON Parse Error: {}",
                    __func__, glz::format_error(ec, res.value().body));
         return false;
      }

      watchedPercent_ = serverResponse.response.data.movie_watched_percent;
      return true;
   }

   int32_t TautulliApi::GetWatchedPercent()
   {
      if (watchedPercent_) return *watchedPercent_;

      constexpr int32_t defaultWatchedPercent = 85;
      return ReadMonitoringData() ? *watchedPercent_ : defaultWatchedPercent;
   }

   std::optional<TautulliHistoryItems> TautulliApi::GetWatchHistory(std::string_view user, const ApiParams& extraParams)
   {
      ApiParams params = {
         GetCmdParam(CMD_GET_HISTORY),
         {INCLUDE_ACTIVITY, "0"},
         {USER, user}
      };
      params.reserve(params.size() + extraParams.size());
      params.insert(params.end(), extraParams.begin(), extraParams.end());

      auto res = client_.Get(BuildApiParamsPath("", params), headers_);
      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      JsonTautulliResponse<JsonTautulliHistoryData> serverResponse;
      if (auto ec = glz::read < glz::opts{.error_on_unknown_keys = false} > (serverResponse, res.value().body))
      {
         LogWarning("{} - JSON Parse Error: {}",
                    __func__, glz::format_error(ec, res.value().body));
         return std::nullopt;
      }

      TautulliHistoryItems history;
      history.items.reserve(serverResponse.response.data.data.size());

      auto watchedPercent = GetWatchedPercent();
      for (auto& item : serverResponse.response.data.data)
      {
         history.items.emplace_back(TautulliHistoryItem{
             .name = std::move(item.title),
             .fullName = std::move(item.full_title),
             .id = item.rating_key,
             .watched = item.percent_complete >= watchedPercent,
             .timeWatchedEpoch = item.stopped,
             .playbackPercentage = item.percent_complete
         });
      }

      return history;
   }

   std::optional<TautulliHistoryItems> TautulliApi::GetWatchHistoryForUser(std::string_view user, std::string_view dateForHistory)
   {
      return GetWatchHistory(user, {
         {AFTER, dateForHistory}
      });
   }

   void TautulliApi::RunSettingsUpdate()
   {
      ReadMonitoringData();
   }
}