#include "tmdb-genre-source.h"

#include <format>
#include <utility>

namespace loomis
{
   TmdbGenreSource::TmdbGenreSource(TmdbPlaylistGenreConfig genre, TmdbMediaType mediaType)
      : genre_(std::move(genre))
      , mediaType_(mediaType)
   {
   }

   std::optional<std::vector<TmdbMediaItem>> TmdbGenreSource::Fetch(
      TmdbClient& client,
      std::size_t maxItems) const
   {
      TmdbQueryParams params{{"sort_by", "popularity.desc"}};
      if (!genre_.withGenres.empty())
         params.emplace_back("with_genres", genre_.withGenres);
      if (!genre_.withoutGenres.empty())
         params.emplace_back("without_genres", genre_.withoutGenres);

      const auto mediaType = GetTmdbMediaTypeString(mediaType_);
      return client.GetMediaItems(
         std::format("/discover/{}", mediaType),
         mediaType_,
         maxItems,
         params);
   }
} // namespace loomis
