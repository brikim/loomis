#include "api-base.h"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

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

   std::string ApiBase::GetPercentEncoded(std::string_view value)
   {
      std::ostringstream escaped;
      escaped.fill('0');
      escaped << std::hex;

      for (unsigned char c : value)
      {
         // Keep alphanumeric and other "unreserved" characters
         if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
         {
            escaped << c;
            continue;
         }

         // Any other characters are percent-encoded
         escaped << std::uppercase;
         escaped << '%' << std::setw(2) << static_cast<int>(c);
         escaped << std::nouppercase;
      }

      return escaped.str();
   }
}