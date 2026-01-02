#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace loomis
{
   inline std::string BuildCommaSeparatedList(const std::vector<std::string>& list)
   {
      if (list.empty()) return "";

      return std::accumulate(std::next(list.begin()), list.end(), list[0],
         [](std::string a, const std::string& b) {
         return std::move(a) + "," + b;
      });
   }

   inline std::string BuildCommaSeparatedList(const std::vector<int32_t>& ids)
   {
      if (ids.empty()) return {};

      std::string result;
      // Optimization: Pre-reserve to avoid reallocations
      result.reserve(ids.size() * 7);

      auto it = ids.begin();
      // Format the first element without a comma
      std::format_to(std::back_inserter(result), "{}", *it);

      // Format subsequent elements with a leading comma
      for (++it; it != ids.end(); ++it)
      {
         std::format_to(std::back_inserter(result), ",{}", *it);
      }

      return result;
   }
}