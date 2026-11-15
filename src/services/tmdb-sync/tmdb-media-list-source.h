#pragma once

#include "services/tmdb-sync/tmdb-media-source.h"

#include <string>

namespace loomis
{
   class TmdbMediaListSource final : public TmdbMediaSource
   {
   public:
      TmdbMediaListSource(std::string endpoint, TmdbMediaType mediaType);

      [[nodiscard]] std::optional<std::vector<TmdbMediaItem>> Fetch(
         TmdbClient& client,
         std::size_t maxItems) const override;

   private:
      std::string endpoint_;
      TmdbMediaType mediaType_;
   };
} // namespace loomis
