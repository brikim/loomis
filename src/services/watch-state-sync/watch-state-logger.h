#pragma once

#include "base.h"

#include <spdlog/spdlog.h>

namespace loomis
{
   class WatchStateLogger
   {
   public:
      WatchStateLogger(Base& parent) : parent_(parent)
      {
      }

      template<typename... Args>
      void LogTrace(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         parent_.LogTrace(fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void LogInfo(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         parent_.LogInfo(fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void LogWarning(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         parent_.LogWarning(fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void LogError(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         parent_.LogError(fmt, std::forward<Args>(args)...);
      }

   private:
      Base& parent_; // Store a reference to the service
   };
}