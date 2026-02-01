#pragma once

#include <warp/base.h>

#include <format>

namespace loomis
{
   class ServiceLogger
   {
   public:
      ServiceLogger(warp::Base& parent) : parent_(parent)
      {
      }

      ~ServiceLogger() = default;

      template<typename... Args>
      void LogTrace(std::format_string<Args...> fmt, Args &&...args)
      {
         parent_.LogTrace(fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void LogInfo(std::format_string<Args...> fmt, Args &&...args)
      {
         parent_.LogInfo(fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void LogWarning(std::format_string<Args...> fmt, Args &&...args)
      {
         parent_.LogWarning(fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void LogError(std::format_string<Args...> fmt, Args &&...args)
      {
         parent_.LogError(fmt, std::forward<Args>(args)...);
      }

   private:
      warp::Base& parent_;
   };
}