#include "tmdb-trending-source.h"

#include <format>
#include <utility>

namespace loomis
{
   TmdbTrendingSource::TmdbTrendingSource(std::string timeWindow)
      : timeWindow_(std::move(timeWindow))
   {
   }

   std::optional<std::vector<int64_t>> TmdbTrendingSource::Fetch(
      TmdbClient& client,
      std::size_t maxItems) const
   {
      return client.GetMediaItems(
         std::format("/trending/{}/{}", TMDB_MEDIA_TYPE_MOVIE, timeWindow_),
         maxItems);
   }
} // namespace loomis
