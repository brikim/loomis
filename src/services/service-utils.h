#pragma once

#include <chrono>
#include <cstdint>
#include <format>
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

   inline int64_t GetEpochTimeForPlexHistory(uint32_t minusDays)
   {
      auto yesterday = std::chrono::system_clock::now() - std::chrono::days(minusDays);

      // Convert to seconds since epoch
      return std::chrono::duration_cast<std::chrono::seconds>(
          yesterday.time_since_epoch()
      ).count();
   }

   inline std::string GetIsoTimeStr(std::chrono::system_clock::time_point tp)
   {
      return std::format("{:%FT%TZ}", tp);
   }

   inline std::string ReplaceMediaPath(std::string_view fullPath, std::string_view oldPath, std::string_view newPath)
   {
      if (fullPath.starts_with(oldPath))
      {
         std::string returnPath{fullPath};
         return returnPath.replace(0, oldPath.length(), newPath);
      }
      return std::string(fullPath);
   }
}