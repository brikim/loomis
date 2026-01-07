#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace loomis
{
   struct JellystateHistoryItem
   {
      std::string name;
      std::string id;
      std::string user;
      std::string dateWatched;
      std::string seriesName;
      std::string episodeId;
   };

   struct JellystatHistoryItems
   {
      std::vector<JellystateHistoryItem> items;
   };
}