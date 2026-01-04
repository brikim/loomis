#include "api-base.h"

#include "logger/log-utils.h"
#include "types.h"

#include <format>

namespace loomis
{
   ApiBase::ApiBase(std::string_view name,
                    std::string_view url,
                    std::string_view apiKey,
                    std::string_view className,
                    std::string_view ansiiCode)
      : Base(className, ansiiCode, name)
      , name_(name)
      , url_(url)
      , apiKey_(apiKey)
   {
   }

   std::optional<std::vector<Task>> ApiBase::GetTaskList()
   {
      return std::nullopt;
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

   void ApiBase::AddApiParam(std::string& url, const std::list<std::pair<std::string_view, std::string_view>>& params) const
   {
      if (params.empty()) return;

      for (const auto& [key, value] : params)
      {
         char separator = (url.find('?') == std::string::npos) ? '?' : '&';

         // Use the encoding utility on the 'value'
         url.append(std::format("{}{}={}", separator, key, GetPercentEncoded(value)));
      }
   }

   std::string ApiBase::GetPercentEncoded(std::string_view src) const
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

   bool ApiBase::IsHttpSuccess(std::string_view name, const httplib::Result& result, bool log)
   {
      std::string error;

      if (result.error() != httplib::Error::Success)
      {
         error = httplib::to_string(result.error());
      }
      else if (result->status >= 400) // Using 400 is more standard than a custom MAX
      {
         error = std::format("Status {}: {} - {}", result->status, result->reason, result->body);
      }
      else
      {
         // Everything passed
         return true;
      }

      if (log) LogWarning("{} - HTTP error {}", name, utils::GetTag("error", error));
      return false;
   }
}