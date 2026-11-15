#pragma once

#include "services/tmdb-sync/tmdb-media-source.h"

#include <string>

namespace loomis
{
   class TmdbMediaListSource final : public TmdbMediaSource
   {
   public:
      explicit TmdbMediaListSource(std::string endpoint);

      [[nodiscard]] std::optional<std::vector<int64_t>> Fetch(
         TmdbClient& client,
         std::size_t maxItems) const override;

   private:
      std::string endpoint_;
   };
} // namespace loomis
