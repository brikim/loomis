#pragma once

#include "services/tmdb-sync/tmdb-client.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace loomis
{
   class TmdbMediaSource
   {
   public:
      virtual ~TmdbMediaSource() = default;

      [[nodiscard]] virtual std::optional<std::vector<int64_t>> Fetch(
         TmdbClient& client,
         std::size_t maxItems) const = 0;
   };
} // namespace loomis
