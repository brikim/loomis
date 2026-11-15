#pragma once

#include "services/tmdb-sync/tmdb-media-source.h"

#include <string>

namespace loomis
{
   class TmdbTrendingSource final : public TmdbMediaSource
   {
   public:
      explicit TmdbTrendingSource(std::string timeWindow);

      [[nodiscard]] std::optional<std::vector<int64_t>> Fetch(
         TmdbClient& client,
         std::size_t maxItems) const override;

   private:
      std::string timeWindow_;
   };
} // namespace loomis
