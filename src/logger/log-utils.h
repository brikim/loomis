#pragma once

#include "types.h"

#include <format>
#include <regex>
#include <string>

namespace loomis::utils
{
   constexpr const char* const ANSI_CODE_START{"\33[38;5;"};
   constexpr const char* const ANSI_CODE_END{"m"};

   static const std::string ANSI_CODE_LOG_HEADER{std::format("{}249{}", ANSI_CODE_START, ANSI_CODE_END)};
   static const std::string ANSI_CODE_LOG_INFO{std::format("{}2{}", ANSI_CODE_START, ANSI_CODE_END)};
   static const std::string ANSI_CODE_LOG_WARNING{std::format("{}3{}", ANSI_CODE_START, ANSI_CODE_END)};
   static const std::string ANSI_CODE_LOG_ERROR{std::format("{}1{}", ANSI_CODE_START, ANSI_CODE_END)};
   static const std::string ANSI_CODE_LOG_CRITICAL{std::format("{}9{}", ANSI_CODE_START, ANSI_CODE_END)};
   static const std::string ANSI_CODE_LOG_DEFAULT{std::format("{}8{}", ANSI_CODE_START, ANSI_CODE_END)};
   static const std::string ANSI_CODE_LOG{std::format("{}15{}", ANSI_CODE_START, ANSI_CODE_END)};
   static const std::string ANSI_CODE_TAG{std::format("{}37{}", ANSI_CODE_START, ANSI_CODE_END)};

   static const std::string ANSI_CODE_PLEX{std::format("{}220{}", ANSI_CODE_START, ANSI_CODE_END)};
   static const std::string ANSI_CODE_EMBY{std::format("{}77{}", ANSI_CODE_START, ANSI_CODE_END)};
   static const std::string ANSI_CODE_JELLYFIN{std::format("{}134{}", ANSI_CODE_START, ANSI_CODE_END)};

   static const std::string ANSI_CODE_SERVICE_PLAYLIST_SYNC{std::format("{}171{}", ANSI_CODE_START, ANSI_CODE_END)};

   static const std::string ANSI_MONITOR_ADDED{std::format("{}33{}", ANSI_CODE_START, ANSI_CODE_END)};
   static const std::string ANSI_MONITOR_PROCESSED{std::format("{}34{}", ANSI_CODE_START, ANSI_CODE_END)};

   static const std::string ANSI_FORMATTED_UNKNOWN("Unknown Server");
   static const std::string ANSI_FORMATTED_PLEX(std::format("{}Plex{}", ANSI_CODE_PLEX, ANSI_CODE_LOG));
   static const std::string ANSI_FORMATTED_EMBY(std::format("{}Emby{}", ANSI_CODE_EMBY, ANSI_CODE_LOG));
   static const std::string ANSI_FORMATTED_JELLYFIN(std::format("{}Jellyfin{}", ANSI_CODE_JELLYFIN, ANSI_CODE_LOG));

   inline std::string GetTag(std::string_view tag, std::string_view value)
   {
      return std::format("{}{}{}={}", ANSI_CODE_TAG, tag, ANSI_CODE_LOG, value);
   }

   inline std::string GetAnsiText(std::string_view text, std::string_view ansiCode)
   {
      return std::format("{}{}{}", ansiCode, text, ANSI_CODE_LOG);
   }

   inline std::string_view GetFormattedPlex()
   {
      return ANSI_FORMATTED_PLEX;
   }

   inline std::string_view GetFormattedEmby()
   {
      return ANSI_FORMATTED_EMBY;
   }

   inline std::string_view GetFormattedJellyfin()
   {
      return ANSI_FORMATTED_JELLYFIN;
   }

   inline std::string GetServiceHeader(std::string_view ansiiCode, std::string_view name)
   {
      return std::format("{}{}{}:", ansiiCode, name, ANSI_CODE_LOG);
   }

   inline std::string_view GetFormattedApiName(ApiType type)
   {
      switch (type)
      {
         case ApiType::PLEX:
            return GetFormattedPlex();
         case ApiType::EMBY:
            return GetFormattedEmby();
         case ApiType::JELLYFIN:
            return GetFormattedJellyfin();
         default:
            return ANSI_FORMATTED_UNKNOWN;
      }
   }

   static std::string GetServerName(std::string_view server, std::string_view serverInstance)
   {
      return serverInstance.empty()
         ? std::string(server)
         : std::format("{}({})", server, serverInstance);
   }

   static std::string BuildTargetString(std::string_view currentTarget, std::string_view newTarget, std::string_view targetInstance)
   {
      if (currentTarget.empty())
      {
         return GetServerName(newTarget, targetInstance);
      }
      else
      {
         return targetInstance.empty()
            ? std::format("{},{}", currentTarget, newTarget)
            : std::format("{},{}({})", currentTarget, newTarget, targetInstance);
      }
   }

   static std::string StripAsciiCharacters(const std::string& data)
   {
      // Strip ansii codes from the log msg
      const std::regex ansii("\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])");
      return std::regex_replace(data, ansii, "");
   }
}