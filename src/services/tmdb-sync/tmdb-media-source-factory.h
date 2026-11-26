#pragma once

#include "services/service-logger.h"
#include "services/tmdb-sync/tmdb-media-source.h"
#include "services/tmdb-sync/tmdb-playlist-type.h"

#include "config-reader/config-reader-types.h"

#include <memory>
#include <string_view>

namespace loomis
{
   [[nodiscard]] std::optional<std::unique_ptr<TmdbMediaSource>> CreateTmdbMediaSource(
      const TmdbPlaylistConfig& config,
      TmdbPlaylistType playlistType,
      ServiceLogger logger,
      std::string_view dryRunText);
} // namespace loomis
