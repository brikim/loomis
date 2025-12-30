#include "api-tautulli.h"

#include "logger/log-utils.h"

#include <format>
#include <ranges>

namespace loomis
{
   static const std::string API_BASE{"/api/v2"};
   static const std::string API_GET_INFO{"get_tautulli_info"};
   static const std::string API_LIBRARIES{"/library/sections/"};

   static constexpr std::string_view ELEM_MEDIA_CONTAINER{"MediaContainer"};
   static constexpr std::string_view ELEM_MEDIA{"Media"};

   static constexpr std::string_view ATTR_NAME{"name"};
   static constexpr std::string_view ATTR_KEY{"key"};
   static constexpr std::string_view ATTR_TITLE{"title"};
   static constexpr std::string_view ATTR_FILE{"file"};

   TautulliApi::TautulliApi(const ServerConfig& serverConfig)
      : ApiBase(serverConfig.name, serverConfig.tracker, "TautulliApi", utils::ANSI_CODE_TAUTULLI)
      , client_(GetUrl())
   {
      constexpr time_t timeoutSec{5};
      client_.set_connection_timeout(timeoutSec);
   }

   std::string TautulliApi::BuildApiPath(std::string_view path)
   {
      return std::format("{}{}", API_BASE, path);
   }

   bool TautulliApi::GetValid()
   {
      //auto res = client_.Get(BuildApiPath(API_SERVERS), headers_);
      //return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
      return false;
   }

   std::optional<std::string> TautulliApi::GetServerReportedName()
   {
      //httplib::Headers header;
      //if (auto res = client_.Get(BuildApiPath(API_SERVERS), header);
      //    res.error() == httplib::Error::Success)
      //{
      //   pugi::xml_document data;
      //   if (data.load_buffer(res.value().body.c_str(), res.value().body.size()).status == pugi::status_ok
      //       && data.child(ELEM_MEDIA_CONTAINER)
      //       && data.child(ELEM_MEDIA_CONTAINER).first_child()
      //       && data.child(ELEM_MEDIA_CONTAINER).first_child().attribute(ATTR_NAME))
      //   {
      //      return data.child(ELEM_MEDIA_CONTAINER).first_child().attribute(ATTR_NAME).as_string();
      //   }
      //   else
      //   {
      //      LogWarning("GetServerReportedName malformed xml reply received");
      //   }
      //}
      return std::nullopt;
   }
}