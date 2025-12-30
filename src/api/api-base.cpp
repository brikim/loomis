#include "api-base.h"

namespace loomis
{
   ApiBase::ApiBase(std::string_view name, const ServerConnectionConfig& serverConnection, std::string_view className, std::string_view ansiiCode)
      : Base(className, ansiiCode, name)
      , name_(name)
      , url_(serverConnection.url)
      , apiKey_(serverConnection.apiKey)
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