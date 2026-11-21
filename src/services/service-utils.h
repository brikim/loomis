#pragma once

#include <chrono>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace loomis
{
   inline constexpr std::string_view GetDryRunText(bool dryRun) noexcept
   {
      return dryRun ? "[DRY RUN] " : "";
   }

   inline auto GetTimePointForHistory(uint32_t minusDays)
   {
      return std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::system_clock::now() - std::chrono::days(minusDays)};
   }

   inline std::string GetDatetimeForHistoryPlex(uint32_t minusDays)
   {
      return std::format("{:%Y-%m-%d}", GetTimePointForHistory(minusDays));
   }

   inline int64_t GetEpochTimeForPlexHistory(uint32_t minusDays)
   {
      auto yesterday = std::chrono::system_clock::now() - std::chrono::days(minusDays);

      // Convert to seconds since epoch
      return std::chrono::duration_cast<std::chrono::seconds>(
                yesterday.time_since_epoch())
         .count();
   }

   inline std::string GetIsoTimeStr(std::chrono::system_clock::time_point tp)
   {
      return std::format("{:%FT%TZ}", tp);
   }
} // namespace loomis
