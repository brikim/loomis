#pragma once

#include "config-reader/config-reader-types.h"

#include <spdlog/fmt/bundled/format.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <vector>

namespace loomis
{
   class Logger
   {
   public:
      // Returns a static instance of the Logger class
      static Logger& Instance()
      {
         static Logger instance;
         return instance;
      }

      void InitApprise(const AppriseLoggingConfig& config);

      template<typename... Args>
      void Trace(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         logger_->trace(fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void TraceWithHeader(std::string_view header, spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         if (logger_->should_log(spdlog::level::trace))
         {
            spdlog::memory_buf_t buf;
            fmt::vformat_to(std::back_inserter(buf), fmt, fmt::make_format_args(args...));
            logger_->trace("{}: {}", header, std::string_view(buf.data(), buf.size()));
         }
      }

      template<typename... Args>
      void Info(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         logger_->info(fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void InfoWithHeader(std::string_view header, spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         spdlog::memory_buf_t buf;
         fmt::vformat_to(std::back_inserter(buf), fmt, fmt::make_format_args(args...));
         logger_->info("{}: {}", header, std::string_view(buf.data(), buf.size()));
      }

      template<typename... Args>
      void Warning(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         logger_->warn(fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void WarningWithHeader(std::string_view header, spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         spdlog::memory_buf_t buf;
         fmt::vformat_to(std::back_inserter(buf), fmt, fmt::make_format_args(args...));
         logger_->warn("{}: {}", header, std::string_view(buf.data(), buf.size()));
      }

      template<typename... Args>
      void Error(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         logger_->error(fmt, std::forward<Args>(args)...);
      }

      template<typename... Args>
      void ErrorWithHeader(std::string_view header, spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         spdlog::memory_buf_t buf;
         fmt::vformat_to(std::back_inserter(buf), fmt, fmt::make_format_args(args...));
         logger_->error("{}: {}", header, std::string_view(buf.data(), buf.size()));
      }

      template<typename... Args>
      void Critical(spdlog::format_string_t<Args...> fmt, Args &&...args)
      {
         logger_->critical(fmt, std::forward<Args>(args)...);
      }

   private:
      Logger();
      virtual ~Logger() = default;

      std::shared_ptr<spdlog::logger> logger_;
   };
}