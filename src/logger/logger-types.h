#pragma once

#include <format>
#include <string>

namespace loomis::log
{
   inline constexpr const std::string_view ANSI_CODE_START{"\33[38;5;"};
   inline constexpr const std::string_view ANSI_CODE_END{"m"};
   inline constexpr const std::string_view ANSI_CODE_RESET{"\033[49m"};

   inline const std::string ANSI_CODE_LOG_HEADER{std::format("{}249{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG_INFO{std::format("{}75{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG_WARNING{std::format("{}214{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG_ERROR{std::format("{}196{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG_CRITICAL{std::format("{}255;48;5;1{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG_DEFAULT{std::format("{}8{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_LOG{std::format("{}15{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_TAG{std::format("{}37{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_STANDOUT{std::format("{}158{}", ANSI_CODE_START, ANSI_CODE_END)};

   inline const std::string ANSI_CODE_PLEX{std::format("{}220{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_EMBY{std::format("{}77{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_TAUTULLI{std::format("{}136{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_JELLYSTAT{std::format("{}63{}", ANSI_CODE_START, ANSI_CODE_END)};

   inline const std::string ANSI_CODE_SERVICE_PLAYLIST_SYNC{std::format("{}171{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_SERVICE_WATCH_STATE_SYNC{std::format("{}45{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_CODE_SERVICE_FOLDER_CLEANUP{std::format("{}173{}", ANSI_CODE_START, ANSI_CODE_END)};

   inline const std::string ANSI_MONITOR_ADDED{std::format("{}33{}", ANSI_CODE_START, ANSI_CODE_END)};
   inline const std::string ANSI_MONITOR_PROCESSED{std::format("{}34{}", ANSI_CODE_START, ANSI_CODE_END)};

   inline const std::string ANSI_FORMATTED_UNKNOWN("Unknown Server");
   inline const std::string ANSI_FORMATTED_PLEX(std::format("{}Plex{}", ANSI_CODE_PLEX, ANSI_CODE_LOG));
   inline const std::string ANSI_FORMATTED_EMBY(std::format("{}Emby{}", ANSI_CODE_EMBY, ANSI_CODE_LOG));
   inline const std::string ANSI_FORMATTED_TAUTULLI(std::format("{}Tautulli{}", ANSI_CODE_TAUTULLI, ANSI_CODE_LOG));
   inline const std::string ANSI_FORMATTED_JELLYSTAT(std::format("{}Jellystat{}", ANSI_CODE_JELLYSTAT, ANSI_CODE_LOG));

   // Log pattern to be used by the logger
   inline const std::string DEFAULT_PATTERN{"%m/%d/%Y %T [%l] %v"};
   inline const std::string DEFAULT_PATTERN_ANSII_INFO{std::format("{}%m/%d/%Y %T {}[{}%l{}] %v{}", ANSI_CODE_LOG_HEADER, ANSI_CODE_LOG, ANSI_CODE_LOG_INFO, ANSI_CODE_LOG, ANSI_CODE_RESET)};
   inline const std::string DEFAULT_PATTERN_ANSII_WARNING{std::format("{}%m/%d/%Y %T {}[{}%l{}] %v{}", ANSI_CODE_LOG_HEADER, ANSI_CODE_LOG, ANSI_CODE_LOG_WARNING, ANSI_CODE_LOG, ANSI_CODE_RESET)};
   inline const std::string DEFAULT_PATTERN_ANSII_ERROR{std::format("{}%m/%d/%Y %T {}[{}%l{}] %v{}", ANSI_CODE_LOG_HEADER, ANSI_CODE_LOG, ANSI_CODE_LOG_ERROR, ANSI_CODE_LOG, ANSI_CODE_RESET)};
   inline const std::string DEFAULT_PATTERN_ANSII_CRITICAL{std::format("{}%m/%d/%Y %T {}[{}%l{}{}] %v{}", ANSI_CODE_LOG_HEADER, ANSI_CODE_LOG, ANSI_CODE_LOG_CRITICAL, ANSI_CODE_RESET, ANSI_CODE_LOG, ANSI_CODE_RESET)};
   inline const std::string DEFAULT_PATTERN_ANSII_DEFAULT{std::format("{}%m/%d/%Y %T {}[{}%l{}] %v{}", ANSI_CODE_LOG_HEADER, ANSI_CODE_LOG, ANSI_CODE_LOG_DEFAULT, ANSI_CODE_LOG, ANSI_CODE_RESET)};
}