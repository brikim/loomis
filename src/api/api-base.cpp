#include "api-base.h"

namespace loomis
{
   ApiBase::ApiBase(const ServerConfig& serverConfig, std::string_view className, std::string_view ansiiCode)
      : Base(className, ansiiCode, serverConfig.name)
      , name_(serverConfig.name)
      , url_(serverConfig.url)
      , apiKey_(serverConfig.apiKey)
   {
   }

   const std::string& ApiBase::GetName() const
   {
      return name_;
   }

   const std::string& ApiBase::GetUrl() const
   {
      return url_;
   }

   const std::string& ApiBase::GetApiKey() const
   {
      return apiKey_;
   }
}