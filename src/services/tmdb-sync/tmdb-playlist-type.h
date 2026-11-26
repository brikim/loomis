#pragma once

#include <optional>
#include <string_view>

namespace loomis
{
   inline constexpr std::string_view TMDB_PLAYLIST_TYPE_TRENDING{"trending"};
   inline constexpr std::string_view TMDB_PLAYLIST_TYPE_POPULAR{"popular"};
   inline constexpr std::string_view TMDB_PLAYLIST_TYPE_TOP_RATED{"top_rated"};
   inline constexpr std::string_view TMDB_PLAYLIST_TYPE_NOW_PLAYING{"now_playing"};
   inline constexpr std::string_view TMDB_PLAYLIST_TYPE_UPCOMING{"upcoming"};
   inline constexpr std::string_view TMDB_PLAYLIST_TYPE_GENRE{"genre"};
   inline constexpr std::string_view TMDB_PLAYLIST_TYPE_DISCOVER{"discover"};

   enum class TmdbPlaylistType
   {
      Trending,
      Popular,
      TopRated,
      NowPlaying,
      Upcoming,
      Genre,
      Discover
   };

   [[nodiscard]] inline std::optional<TmdbPlaylistType> ParseTmdbPlaylistType(std::string_view type)
   {
      if (type == TMDB_PLAYLIST_TYPE_TRENDING)
         return TmdbPlaylistType::Trending;
      if (type == TMDB_PLAYLIST_TYPE_POPULAR)
         return TmdbPlaylistType::Popular;
      if (type == TMDB_PLAYLIST_TYPE_TOP_RATED)
         return TmdbPlaylistType::TopRated;
      if (type == TMDB_PLAYLIST_TYPE_NOW_PLAYING)
         return TmdbPlaylistType::NowPlaying;
      if (type == TMDB_PLAYLIST_TYPE_UPCOMING)
         return TmdbPlaylistType::Upcoming;
      if (type == TMDB_PLAYLIST_TYPE_GENRE)
         return TmdbPlaylistType::Genre;
      if (type == TMDB_PLAYLIST_TYPE_DISCOVER)
         return TmdbPlaylistType::Discover;

      return std::nullopt;
   }
} // namespace loomis
