#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace loomis
{
   inline auto GetTimePointForHistory(uint32_t minusDays)
   {
      return std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now() - std::chrono::days(minusDays)};
   }

   inline std::string GetDatetimeForHistoryPlex(uint32_t minusDays)
   {
      return std::format("{:%Y-%m-%d}", GetTimePointForHistory(minusDays));
   }
}