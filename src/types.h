#pragma once

#include <functional>
#include <string>

namespace loomis
{
   using sys_clk = std::chrono::system_clock;

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
      bool service{false};
      std::string name;
      std::string cronExpression;
      std::function<void()> func;
   };
}