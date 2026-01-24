#pragma once

#include <warp/log-types.h>

#include <format>
#include <string>

namespace loomis
{
   inline const std::string ANSI_CODE_SERVICE_PLAYLIST_SYNC{std::format("{}171{}", warp::ANSI_CODE_START, warp::ANSI_CODE_END)};
   inline const std::string ANSI_CODE_SERVICE_WATCH_STATE_SYNC{std::format("{}45{}", warp::ANSI_CODE_START, warp::ANSI_CODE_END)};
   inline const std::string ANSI_CODE_SERVICE_FOLDER_CLEANUP{std::format("{}173{}", warp::ANSI_CODE_START, warp::ANSI_CODE_END)};
   inline const std::string ANSI_CODE_SERVICE_DVR_MAINTAINER{std::format("{}205{}", warp::ANSI_CODE_START, warp::ANSI_CODE_END)};
}