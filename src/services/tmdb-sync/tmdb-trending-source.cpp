#include "tmdb-trending-source.h"

#include <format>
#include <utility>

namespace loomis
{
   TmdbTrendingSource::TmdbTrendingSource(std::string timeWindow, TmdbMediaType mediaType)
      : timeWindow_(std::move(timeWindow))
      , mediaType_(mediaType)
   {
   }

   std::optional<std::vector<TmdbMediaItem>> TmdbTrendingSource::Fetch(
      TmdbClient& client,
      std::size_t maxItems) const
   {
      const auto mediaType = GetTmdbMediaTypeString(mediaType_);
      return client.GetMediaItems(
         std::format("/trending/{}/{}", mediaType, timeWindow_),
         mediaType_,
         maxItems);
   }
} // namespace loomis
