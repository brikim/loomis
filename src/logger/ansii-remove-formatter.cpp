#include "ansii-remove-formatter.h"

#include "logger/logger-types.h"
#include "logger/log-utils.h"

namespace loomis
{
   AnsiiRemoveFormatter::AnsiiRemoveFormatter()
   {
      patternFormatter_.set_pattern(log::DEFAULT_PATTERN);
   }

   void AnsiiRemoveFormatter::format(const spdlog::details::log_msg& msg, spdlog::memory_buf_t& dest)
   {
      // Create a new log message with the new string
      // Data is not initialized memory so need to create a new string using the size of the payload
      auto formattedMsg = log::StripAsciiCharacters(std::string(msg.payload.data(), msg.payload.size()));
      spdlog::details::log_msg newMsg(msg.time, msg.source, msg.logger_name, msg.level, formattedMsg);

      // Use the normal pattern formatter to create the log message with no ansii codes
      patternFormatter_.format(newMsg, dest);
   }

   std::unique_ptr<spdlog::formatter> AnsiiRemoveFormatter::clone() const
   {
      return spdlog::details::make_unique<AnsiiRemoveFormatter>();
   }
}