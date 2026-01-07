#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace loomis
{
   struct JsonHistoryResult
   {
      std::string NowPlayingItemName;
      std::string NowPlayingItemId;
      std::string UserName;
      std::string ActivityDateInserted;
      std::optional<std::string> SeriesName;
      std::optional<std::string> EpisodeId;
   };

   struct JsonHistoryResults
   {
      std::vector<JsonHistoryResult> results;
   };
}