#pragma once

#include "services/tmdb-sync/tmdb-media-source.h"

#include "config-reader/config-reader-types.h"

namespace loomis
{
   class TmdbDiscoverSource final : public TmdbMediaSource
   {
   public:
      explicit TmdbDiscoverSource(TmdbPlaylistDiscoverConfig discover);

      [[nodiscard]] std::optional<std::vector<int64_t>> Fetch(
         TmdbClient& client,
         std::size_t maxItems) const override;

   private:
      TmdbPlaylistDiscoverConfig discover_;
   };
} // namespace loomis
