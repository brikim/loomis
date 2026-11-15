#pragma once

#include "services/tmdb-sync/tmdb-media-source.h"

#include <string>

namespace loomis
{
   class TmdbTrendingSource final : public TmdbMediaSource
   {
   public:
      TmdbTrendingSource(std::string timeWindow, TmdbMediaType mediaType);

      [[nodiscard]] std::optional<std::vector<TmdbMediaItem>> Fetch(
         TmdbClient& client,
         std::size_t maxItems) const override;

   private:
      std::string timeWindow_;
      TmdbMediaType mediaType_;
   };
} // namespace loomis
