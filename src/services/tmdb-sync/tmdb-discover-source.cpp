#include "tmdb-discover-source.h"

#include <utility>

namespace loomis
{
   namespace
   {
      constexpr std::string_view DISCOVER_MOVIE_ENDPOINT{"/discover/movie"};
      constexpr std::string_view DEFAULT_SORT{"popularity.desc"};
   } // namespace

   TmdbDiscoverSource::TmdbDiscoverSource(TmdbPlaylistDiscoverConfig discover)
      : discover_(std::move(discover))
   {
   }

   std::optional<std::vector<int64_t>> TmdbDiscoverSource::Fetch(
      TmdbClient& client,
      std::size_t maxItems) const
   {
      TmdbQueryParams params{{"sort_by", DEFAULT_SORT}};
      if (!discover_.withKeywords.empty())
         params.emplace_back("with_keywords", discover_.withKeywords);
      if (!discover_.withoutKeywords.empty())
         params.emplace_back("without_keywords", discover_.withoutKeywords);

      return client.GetMediaItems(
         DISCOVER_MOVIE_ENDPOINT,
         maxItems,
         params);
   }
} // namespace loomis
