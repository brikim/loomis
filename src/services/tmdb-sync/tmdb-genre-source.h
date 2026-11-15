#pragma once

#include "services/tmdb-sync/tmdb-media-source.h"

#include "config-reader/config-reader-types.h"

namespace loomis
{
   class TmdbGenreSource final : public TmdbMediaSource
   {
   public:
      explicit TmdbGenreSource(TmdbPlaylistGenreConfig genre);

      [[nodiscard]] std::optional<std::vector<int64_t>> Fetch(
         TmdbClient& client,
         std::size_t maxItems) const override;

   private:
      TmdbPlaylistGenreConfig genre_;
   };
} // namespace loomis
