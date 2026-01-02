#pragma once

#include <functional>
#include <string>

namespace loomis
{
   static constexpr int VALID_HTTP_RESPONSE_MAX{300};

   enum class LogType
   {
      TRACE,
      INFO,
      WARN,
      ERR
   };

   enum class ApiType
   {
      PLEX,
      EMBY,
      TAUTULLI,
      JELLYSTAT
   };

   struct Task
   {
      std::string name;
      std::string cronExpression;
      std::function<void()> func;
   };
}