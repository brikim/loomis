#include "tmdb-genre-source.h"

#include <format>
#include <utility>

namespace loomis
{
   namespace
   {
      constexpr std::string_view DISCOVER_MOVIE_ENDPOINT{"/discover/movie"};
   }

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
         DISCOVER_MOVIE_ENDPOINT,
         maxItems,
         params);
   }
} // namespace loomis
