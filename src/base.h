#pragma once

#include "logger/logger.h"
#include "types.h"

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <optional>
#include <string>

namespace loomis
{
   class Base
   {
   public:
      Base(std::string_view className, std::string_view ansiiCode, std::optional<std::string_view> classExtra);
      virtual ~Base() = default;

      template<typename... Args>
      void LogTrace(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         Logger::Instance().TraceWithHeader(header_, fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void LogInfo(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         Logger::Instance().InfoWithHeader(header_, fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void LogWarning(spdlog::format_string_t<Args...> fmt, Args&&... args)
      {
         Logger::Instance().WarningWithHeader(header_, fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void LogError(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         Logger::Instance().ErrorWithHeader(header_, fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void Log(LogType type, std::string_view fmt, Args &&...args)
      {
         switch (type)
         {
            case LogType::TRACE:
               LogTrace(fmt, std::forward<Args>(args)...);
               break;
            case LogType::INFO:
               LogInfo(fmt, std::forward<Args>(args)...);
               break;
            case LogType::WARN:
               LogWarning(fmt, std::forward<Args>(args)...);
               break;
            case LogType::ERR:
               LogError(fmt, std::forward<Args>(args)...);
               break;
            default:
               break;
         }
      }

   private:
      std::string header_;
   };
}