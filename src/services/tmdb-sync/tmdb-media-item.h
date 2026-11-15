#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace loomis
{
   inline constexpr int64_t INVALID_TMDB_ID{0};
   inline constexpr std::string_view TMDB_MEDIA_TYPE_MOVIE{"movie"};
   inline constexpr std::string_view TMDB_MEDIA_TYPE_TV{"tv"};
   inline constexpr std::string_view TMDB_MEDIA_TYPE_ALL{"all"};
   inline constexpr std::array<std::string_view, 3> TMDB_MEDIA_TYPE_NAMES{
      TMDB_MEDIA_TYPE_MOVIE,
      TMDB_MEDIA_TYPE_TV,
      TMDB_MEDIA_TYPE_ALL};

   enum class TmdbMediaType
   {
      Movie,
      Tv,
      All
   };

   inline constexpr std::optional<TmdbMediaType> ParseTmdbMediaType(std::string_view mediaType)
   {
      if (mediaType == TMDB_MEDIA_TYPE_MOVIE)
         return TmdbMediaType::Movie;
      if (mediaType == TMDB_MEDIA_TYPE_TV)
         return TmdbMediaType::Tv;
      if (mediaType == TMDB_MEDIA_TYPE_ALL)
         return TmdbMediaType::All;

      return std::nullopt;
   }

   inline constexpr std::string_view GetTmdbMediaTypeString(TmdbMediaType mediaType)
   {
      switch (mediaType)
      {
      case TmdbMediaType::Movie:
         return TMDB_MEDIA_TYPE_MOVIE;
      case TmdbMediaType::Tv:
         return TMDB_MEDIA_TYPE_TV;
      case TmdbMediaType::All:
         return TMDB_MEDIA_TYPE_ALL;
      default:
         return {};
      }
   }

   struct TmdbMediaItem
   {
      int64_t id{INVALID_TMDB_ID};
      TmdbMediaType mediaType{TmdbMediaType::Movie};
   };
} // namespace loomis
