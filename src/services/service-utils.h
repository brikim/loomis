#pragma once

#include <chrono>
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

   inline std::string GetIsoTimeStr(std::chrono::system_clock::time_point tp)
   {
      return std::format("{:%FT%TZ}", tp);
   }

   inline std::string ReplaceMediaPath(const std::string& fullPath, const std::string& oldPath, const std::string& newPath)
   {
      if (fullPath.starts_with(oldPath))
      {
         auto returnPath = fullPath;
         return returnPath.replace(0, oldPath.length(), newPath);
      }
      return fullPath;
   }
}