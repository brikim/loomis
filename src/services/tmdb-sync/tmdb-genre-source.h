#pragma once

#include "services/tmdb-sync/tmdb-media-source.h"

#include "config-reader/config-reader-types.h"

namespace loomis
{
   class TmdbGenreSource final : public TmdbMediaSource
   {
   public:
      TmdbGenreSource(TmdbPlaylistGenreConfig genre, TmdbMediaType mediaType);

      [[nodiscard]] std::optional<std::vector<TmdbMediaItem>> Fetch(
         TmdbClient& client,
         std::size_t maxItems) const override;

   private:
      TmdbPlaylistGenreConfig genre_;
      TmdbMediaType mediaType_;
   };
} // namespace loomis
