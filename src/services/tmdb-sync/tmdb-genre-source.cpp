#include "tmdb-genre-source.h"

#include <format>
#include <utility>

namespace loomis
{
   TmdbGenreSource::TmdbGenreSource(TmdbPlaylistGenreConfig genre)
      : genre_(std::move(genre))
   {
   }

   std::optional<std::vector<int64_t>> TmdbGenreSource::Fetch(
      TmdbClient& client,
      std::size_t maxItems) const
   {
      TmdbQueryParams params{{"sort_by", "popularity.desc"}};
      if (!genre_.withGenres.empty())
         params.emplace_back("with_genres", genre_.withGenres);
      if (!genre_.withoutGenres.empty())
         params.emplace_back("without_genres", genre_.withoutGenres);

      return client.GetMediaItems(
         std::format("/discover/{}", TMDB_MEDIA_TYPE_MOVIE),
         maxItems,
         params);
   }
} // namespace loomis
