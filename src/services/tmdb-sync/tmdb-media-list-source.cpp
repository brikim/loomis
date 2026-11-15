#include "tmdb-media-list-source.h"

#include <utility>

namespace loomis
{
   TmdbMediaListSource::TmdbMediaListSource(std::string endpoint, TmdbMediaType mediaType)
      : endpoint_(std::move(endpoint))
      , mediaType_(mediaType)
   {
   }

   std::optional<std::vector<TmdbMediaItem>> TmdbMediaListSource::Fetch(
      TmdbClient& client,
      std::size_t maxItems) const
   {
      return client.GetMediaItems(endpoint_, mediaType_, maxItems);
   }
} // namespace loomis
