#include "base.h"

#include "logger/log-utils.h"

#include <format>

namespace loomis
{
   Base::Base(std::string_view className, std::string_view ansiiCode, std::optional<std::string_view> classExtra)
      : header_(classExtra.has_value()
                ? std::format("{}{}{}({})", ansiiCode, className, utils::ANSI_CODE_LOG, classExtra.value())
                : utils::GetServiceHeader(ansiiCode, className))
   {
   }
}