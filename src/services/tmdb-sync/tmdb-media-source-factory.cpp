#include "tmdb-media-source-factory.h"

#include "services/tmdb-sync/tmdb-genre-source.h"
#include "services/tmdb-sync/tmdb-media-list-source.h"
#include "services/tmdb-sync/tmdb-trending-source.h"
#include "services/tmdb-sync/tmdb-string-utils.h"

#include <warp/log/log-utils.h>

#include <algorithm>
#include <array>
#include <format>

namespace loomis
{
   namespace
   {
      constexpr std::array<std::string_view, 2> VALID_TIME_WINDOWS{"day", "week"};
   } // namespace

   std::unique_ptr<TmdbMediaSource> CreateTmdbMediaSource(
      const TmdbPlaylistConfig& config,
      TmdbPlaylistType playlistType,
      TmdbMediaType mediaType,
      ServiceLogger logger,
      std::string_view dryRunText)
   {
      if (mediaType == TmdbMediaType::All && playlistType != TmdbPlaylistType::Trending)
      {
         logger.LogError(
            "{}Media type '{}' is only supported for {} playlists.",
            dryRunText,
            TMDB_MEDIA_TYPE_ALL,
            TMDB_PLAYLIST_TYPE_TRENDING);
         return nullptr;
      }

      const auto mediaTypeName = GetTmdbMediaTypeString(mediaType);
      const auto createMediaListSource = [&](std::string_view endpointType) {
         return std::make_unique<TmdbMediaListSource>(
            std::format("/{}/{}", mediaTypeName, endpointType),
            mediaType);
      };

      switch (playlistType)
      {
      case TmdbPlaylistType::Trending:
         if (!config.timeWindow.has_value())
         {
            logger.LogError(
               "{}{} is {} and requires a time window. Valid time windows are: {}",
               dryRunText,
               warp::GetTag("playlist", config.name),
               warp::GetTag("type", TMDB_PLAYLIST_TYPE_TRENDING),
               JoinCommaSeparated(VALID_TIME_WINDOWS));
            return nullptr;
         }

         if (std::ranges::find(VALID_TIME_WINDOWS, *config.timeWindow) == VALID_TIME_WINDOWS.end())
         {
            logger.LogError(
               "{}{} has invalid {}. Valid time windows are: {}",
               dryRunText,
               warp::GetTag("playlist", config.name),
               warp::GetTag("time window", *config.timeWindow),
               JoinCommaSeparated(VALID_TIME_WINDOWS));
            return nullptr;
         }

         return std::make_unique<TmdbTrendingSource>(*config.timeWindow, mediaType);
      case TmdbPlaylistType::Popular:
         return createMediaListSource(TMDB_PLAYLIST_TYPE_POPULAR);
      case TmdbPlaylistType::TopRated:
         return createMediaListSource(TMDB_PLAYLIST_TYPE_TOP_RATED);
      case TmdbPlaylistType::NowPlaying:
      case TmdbPlaylistType::Upcoming:
         if (mediaType != TmdbMediaType::Movie)
         {
            logger.LogError(
               "{}{} has {} which is only available for movies. Use {} or {} for TV.",
               dryRunText,
               warp::GetTag("playlist", config.name),
               warp::GetTag("playlist type", config.type),
               TMDB_PLAYLIST_TYPE_AIRING_TODAY,
               TMDB_PLAYLIST_TYPE_ON_THE_AIR);
            return nullptr;
         }
         if (playlistType == TmdbPlaylistType::NowPlaying)
            return createMediaListSource(TMDB_PLAYLIST_TYPE_NOW_PLAYING);
         return createMediaListSource(TMDB_PLAYLIST_TYPE_UPCOMING);
      case TmdbPlaylistType::AiringToday:
      case TmdbPlaylistType::OnTheAir:
         if (mediaType != TmdbMediaType::Tv)
         {
            logger.LogError(
               "{}{} has {} which is only available for TV.",
               dryRunText,
               warp::GetTag("playlist", config.name),
               warp::GetTag("playlist type", config.type));
            return nullptr;
         }
         if (playlistType == TmdbPlaylistType::AiringToday)
            return createMediaListSource(TMDB_PLAYLIST_TYPE_AIRING_TODAY);
         return createMediaListSource(TMDB_PLAYLIST_TYPE_ON_THE_AIR);
      case TmdbPlaylistType::Genre:
         if (!config.genre.has_value())
         {
            logger.LogError(
               "{}{} has no genre filters given. Expected a genre object with 'with' and/or 'without' values.",
               dryRunText,
               warp::GetTag("playlist", config.name));
            return nullptr;
         }

         if (config.genre->withGenres.empty() && config.genre->withoutGenres.empty())
         {
            logger.LogError(
               "{}{} genre filter has no values. Provide at least one 'with' or 'without' genre id.",
               dryRunText,
               warp::GetTag("playlist", config.name));
            return nullptr;
         }

         return std::make_unique<TmdbGenreSource>(*config.genre, mediaType);
      default:
         logger.LogError(
            "{}{} has {} which is not supported for {}.",
            dryRunText,
            warp::GetTag("playlist", config.name),
            warp::GetTag("playlist type", config.type),
            warp::GetTag("media type", GetTmdbMediaTypeString(mediaType)));
         return nullptr;
      }
   }
} // namespace loomis
