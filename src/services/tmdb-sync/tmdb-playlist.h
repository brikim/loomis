#pragma once

#include "services/service-logger.h"
#include "services/tmdb-sync/tmdb-client.h"
#include "services/tmdb-sync/tmdb-media-source.h"

#include "config-reader/config-reader-types.h"

#include <memory>
#include <string>

namespace loomis
{
   class TmdbPlaylist
   {
   public:
      TmdbPlaylist(const TmdbPlaylistConfig& config,
                   std::shared_ptr<TmdbClient> tmdbClient,
                   bool dryRun,
                   ServiceLogger logger);
      ~TmdbPlaylist() = default;

      [[nodiscard]] bool IsValid() const;

      void Sync();

   private:
      void ErrorCheckConfig();

      bool valid_{false};

      TmdbPlaylistConfig config_;
      std::shared_ptr<TmdbClient> tmdbClient_;
      std::unique_ptr<TmdbMediaSource> mediaSource_;
      ServiceLogger logger_;
      std::string dryRunText_;
   };
} // namespace loomis
