#pragma once

#include <cstddef>
#include <string>

namespace loomis
{
   struct TmdbSyncStats
   {
      std::size_t added{0u};
      std::size_t removed{0u};
      bool reordered{false};

      bool operator==(const TmdbSyncStats&) const = default;
   };

   struct TmdbSyncLogEntry
   {
      std::string destination;
      TmdbSyncStats stats;
   };
} // namespace loomis
