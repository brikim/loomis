#pragma once

#include <functional>
#include <string>

namespace loomis
{
   static constexpr int VALID_HTTP_RESPONSE_MAX{300};

   enum class ApiType
   {
      PLEX,
      EMBY,
      JELLYFIN
   };

   struct Task
   {
      std::string name;
      std::string cronExpression;
      std::function<void()> func;
   };
}