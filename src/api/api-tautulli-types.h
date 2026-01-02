#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace loomis
{
   struct TautulliUserInfo
   {
      int32_t id{0};
      bool friendlyNameValid{false};
      std::string friendlyName;
   };

   // data representing an Tautulli History item
   struct TautulliHistoryItem
   {
      std::string name;
      std::string fullName;
      int32_t id;
      bool watched;
      int64_t timeWatchedEpoch;
      int32_t playbackPercentage;
   };

   // data representing an Tautulli History items
   struct TautulliHistoryItems
   {
      std::vector<TautulliHistoryItem> items;
   };
}