#include "tmdb-media-source-factory.h"

#include "services/tmdb-sync/tmdb-genre-source.h"
#include "services/tmdb-sync/tmdb-media-list-source.h"
#include "services/tmdb-sync/tmdb-string-utils.h"

#include <warp/log/log-utils.h>

#include <algorithm>
#include <array>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace loomis
{
   namespace
   {
      constexpr std::array<std::string_view, 2> VALID_TIME_WINDOWS{"day", "week"};

      bool ValidateTrendingConfig(
         const TmdbPlaylistConfig& config,
         ServiceLogger logger,
         std::string_view dryRunText)
      {
         if (!config.timeWindow.has_value())
         {
            logger.LogError(
               "{}{} is {} and requires a time window. Valid time windows are: {}",
               dryRunText,
               warp::GetTag("playlist", config.name),
               warp::GetTag("type", TMDB_PLAYLIST_TYPE_TRENDING),
               JoinCommaSeparated(VALID_TIME_WINDOWS));
            return false;
         }

         if (std::ranges::find(VALID_TIME_WINDOWS, *config.timeWindow) == VALID_TIME_WINDOWS.end())
         {
            logger.LogError(
               "{}{} has invalid {}. Valid time windows are: {}",
               dryRunText,
               warp::GetTag("playlist", config.name),
               warp::GetTag("time window", *config.timeWindow),
               JoinCommaSeparated(VALID_TIME_WINDOWS));
            return false;
         }

         return true;
      }

      bool ValidateGenreConfig(
         const TmdbPlaylistConfig& config,
         ServiceLogger logger,
         std::string_view dryRunText)
      {
         if (!config.genre.has_value())
         {
            logger.LogError(
               "{}{} has no genre filters given. Expected a genre object with 'with' and/or 'without' values.",
               dryRunText,
               warp::GetTag("playlist", config.name));
            return false;
         }

         if (config.genre->withGenres.empty() && config.genre->withoutGenres.empty())
         {
            logger.LogError(
               "{}{} genre filter has no values. Provide at least one 'with' or 'without' genre id.",
               dryRunText,
               warp::GetTag("playlist", config.name));
            return false;
         }

         return true;
      }
   } // namespace

   std::unique_ptr<TmdbMediaSource> CreateTmdbMediaSource(
      const TmdbPlaylistConfig& config,
      TmdbPlaylistType playlistType,
      ServiceLogger logger,
      std::string_view dryRunText)
   {
      const auto createMediaListSource = [](std::string endpoint) {
         return std::make_unique<TmdbMediaListSource>(std::move(endpoint));
      };

      switch (playlistType)
      {
      case TmdbPlaylistType::Trending:
         if (!ValidateTrendingConfig(config, logger, dryRunText))
            return nullptr;
         return createMediaListSource(std::format("/trending/movie/{}", *config.timeWindow));

      case TmdbPlaylistType::Popular:
         return createMediaListSource("/movie/popular");

      case TmdbPlaylistType::TopRated:
         return createMediaListSource("/movie/top_rated");

      case TmdbPlaylistType::NowPlaying:
         return createMediaListSource("/movie/now_playing");

      case TmdbPlaylistType::Upcoming:
         return createMediaListSource("/movie/upcoming");

      case TmdbPlaylistType::Genre:
         if (!ValidateGenreConfig(config, logger, dryRunText))
            return nullptr;
         return std::make_unique<TmdbGenreSource>(*config.genre);

      default:
         logger.LogError(
            "{}{} has unsupported {}.",
            dryRunText,
            warp::GetTag("playlist", config.name),
            warp::GetTag("playlist type", config.type));
         return nullptr;
      }
   }
} // namespace loomis
