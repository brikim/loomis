#pragma once

#include "types.h"

#include <format>
#include <regex>
#include <string>

namespace loomis::utils
{
   inline constexpr const char* const ANSI_CODE_START{"\33[38;5;"};
   inline constexpr const char* const ANSI_CODE_END{"m"};

   inline const std::string ANSI_CODE_LOG_HEADER{std::format("{}249{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG_INFO{std::format("{}2{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG_WARNING{std::format("{}3{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG_ERROR{std::format("{}1{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG_CRITICAL{std::format("{}9{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG_DEFAULT{std::format("{}8{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG{std::format("{}15{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_TAG{std::format("{}37{}", ANSI_CODE_START, ANSI_CODE_END)};

   inline const std::string ANSI_CODE_PLEX{std::format("{}220{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_EMBY{std::format("{}77{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_TAUTULLI{std::format("{}136{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_JELLYSTAT{std::format("{}63{}", ANSI_CODE_START, ANSI_CODE_END)};

   inline const std::string ANSI_CODE_SERVICE_PLAYLIST_SYNC{std::format("{}171{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_SERVICE_WATCH_STATE_SYNC{std::format("{}45{}", ANSI_CODE_START, ANSI_CODE_END)};

   inline const std::string ANSI_MONITOR_ADDED{std::format("{}33{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_MONITOR_PROCESSED{std::format("{}34{}", ANSI_CODE_START, ANSI_CODE_END)};

   inline const std::string ANSI_FORMATTED_UNKNOWN("Unknown Server");
   inline const std::string ANSI_FORMATTED_PLEX(std::format("{}Plex{}", ANSI_CODE_PLEX, ANSI_CODE_LOG));
   inline const std::string ANSI_FORMATTED_EMBY(std::format("{}Emby{}", ANSI_CODE_EMBY, ANSI_CODE_LOG));
   inline const std::string ANSI_FORMATTED_TAUTULLI(std::format("{}Tautulli{}", ANSI_CODE_TAUTULLI, ANSI_CODE_LOG));
   inline const std::string ANSI_FORMATTED_JELLYSTAT(std::format("{}Jellystat{}", ANSI_CODE_JELLYSTAT, ANSI_CODE_LOG));

   template <typename T>
   inline std::string GetTag(std::string_view tag, const T& value)
   {
      // std::format will handle converting the value to a string 
      // regardless of whether it is a string, int, or bool.
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

   inline std::string_view GetFormattedTautulli()
   {
      return ANSI_FORMATTED_TAUTULLI;
   }

   inline std::string_view GetFormattedJellystat()
   {
      return ANSI_FORMATTED_JELLYSTAT;
   }

   inline std::string GetServiceHeader(std::string_view ansiiCode, std::string_view name)
   {
      return std::format("{}{}{}", ansiiCode, name, ANSI_CODE_LOG);
   }

   inline std::string_view GetFormattedApiName(ApiType type)
   {
      switch (type)
      {
         case ApiType::PLEX:
            return GetFormattedPlex();
         case ApiType::EMBY:
            return GetFormattedEmby();
         case ApiType::TAUTULLI:
            return GetFormattedTautulli();
         case ApiType::JELLYSTAT:
            return GetFormattedJellystat();
         default:
            return ANSI_FORMATTED_UNKNOWN;
      }
   }

   inline std::string GetServerName(std::string_view server, std::string_view serverInstance)
   {
      return serverInstance.empty()
         ? std::string(server)
         : std::format("{}({})", server, serverInstance);
   }

   inline std::string BuildSyncServerString(std::string_view currentServerList, std::string_view newServer, std::string_view newServerInstance)
   {
      if (currentServerList.empty())
      {
         return GetServerName(newServer, newServerInstance);
      }
      else
      {
         return newServer.empty()
            ? std::format("{},{}", currentServerList, newServer)
            : std::format("{},{}({})", currentServerList, newServer, newServerInstance);
      }
   }

   inline std::string StripAsciiCharacters(const std::string& data)
   {
      // Strip ansii codes from the log msg
      const std::regex ansii("\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])");
      return std::regex_replace(data, ansii, "");
   }
}