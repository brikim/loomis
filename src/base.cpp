#include "base.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

#include <format>

namespace loomis
{
   Base::Base(std::string_view className, std::string_view ansiiCode, std::optional<std::string_view> classExtra)
      : logHeader_(classExtra.has_value()
                   ? std::format("{}{}{}({}):", ansiiCode, className, utils::ANSI_CODE_LOG, classExtra.value())
                   : utils::GetServiceHeader(ansiiCode, className))
   {
   }

   void Base::LogTrace(const std::string& msg)
   {
      Logger::Instance().Trace(std::format("{} {}", logHeader_, msg));
   }

   void Base::LogInfo(const std::string& msg)
   {
      Logger::Instance().Info(std::format("{} {}", logHeader_, msg));
   }

   void Base::LogWarning(const std::string& msg)
   {
      Logger::Instance().Warning(std::format("{} {}", logHeader_, msg));
   }

   void Base::LogError(const std::string& msg)
   {
      Logger::Instance().Error(std::format("{} {}", logHeader_, msg));
   }
}