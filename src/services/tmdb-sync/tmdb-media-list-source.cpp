#include "tmdb-media-list-source.h"

#include <utility>

namespace loomis
{
   TmdbMediaListSource::TmdbMediaListSource(std::string endpoint)
      : endpoint_(std::move(endpoint))
   {
   }

   std::optional<std::vector<int64_t>> TmdbMediaListSource::Fetch(
      TmdbClient& client,
      std::size_t maxItems) const
   {
      return client.GetMediaItems(endpoint_, maxItems);
   }
} // namespace loomis
