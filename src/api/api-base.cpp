#include "api-base.h"

#include "logger/log-utils.h"
#include "types.h"

#include <format>

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

   std::string ApiBase::GetPercentEncoded(std::string_view src)
   {
      // 1. Lookup table for "unreserved" characters (RFC 3986)
      // 0 = needs encoding, 1 = safe
      static const bool SAFE[256] = {
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0-31
          0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0, // 32-63 (Keep . -)
          0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1, // 64-95 (Keep _)
          0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0, // 96-127 (Keep ~)
          // ... all others (128-255) are 0
      };

      static const char hex_chars[] = "0123456789ABCDEF";

      // 2. Pre-calculate exact size to avoid reallocations
      size_t new_size{0};
      for (unsigned char c : src)
      {
         new_size += SAFE[c] ? 1 : 3;
      }

      // 3. One single allocation
      std::string result;
      result.reserve(new_size);

      // 4. Direct pointer-style insertion
      for (unsigned char c : src)
      {
         if (SAFE[c])
         {
            result.push_back(c);
         }
         else
         {
            result.push_back('%');
            result.push_back(hex_chars[c >> 4]);   // High nibble
            result.push_back(hex_chars[c & 0x0F]); // Low nibble
         }
      }

      return result;
   }

   bool ApiBase::IsHttpSuccess(std::string_view name, const httplib::Result& result)
   {
      if (result.error() != httplib::Error::Success || result.value().status >= VALID_HTTP_RESPONSE_MAX)
      {
         LogWarning(std::format("{} - HTTP error {}",
                                name,
                                utils::GetTag("error",
                                              result.value().status >= VALID_HTTP_RESPONSE_MAX ? std::format("{} - {}", result.value().reason, result.value().body) : httplib::to_string(result.error()))));
         return false;
      }
      return true;
   }
}