#pragma once

#include <span>
#include <string>
#include <string_view>

namespace loomis
{
   inline std::string JoinCommaSeparated(std::span<const std::string_view> values)
   {
      if (values.empty())
         return {};

      std::string result(values.front());
      for (const auto value : values.subspan(1))
      {
         result.append(", ");
         result.append(value);
      }

      return result;
   }
} // namespace loomis
