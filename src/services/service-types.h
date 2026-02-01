#pragma once

#include <warp/log/log-types.h>

#include <format>
#include <string>

namespace loomis
{
   inline const std::string ANSI_CODE_SERVICE_PLAYLIST_SYNC{std::format("{}177{}", warp::ANSI_CODE_START, warp::ANSI_CODE_END)};
   inline const std::string ANSI_CODE_SERVICE_WATCH_STATE_SYNC{std::format("{}45{}", warp::ANSI_CODE_START, warp::ANSI_CODE_END)};
   inline const std::string ANSI_CODE_SERVICE_FOLDER_CLEANUP{std::format("{}226{}", warp::ANSI_CODE_START, warp::ANSI_CODE_END)};
   inline const std::string ANSI_CODE_SERVICE_DVR_MAINTAINER{std::format("{}154{}", warp::ANSI_CODE_START, warp::ANSI_CODE_END)};
   inline const std::string ANSI_CODE_SERVICE_DELETE_WATCHED{std::format("{}215{}", warp::ANSI_CODE_START, warp::ANSI_CODE_END)};
}