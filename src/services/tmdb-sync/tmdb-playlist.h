#pragma once

#include "services/service-logger.h"

#include <warp/api/api-manager.h>

#include "config-reader/config-reader-types.h"

#include <string>

namespace loomis
{
   class TmdbPlaylist
   {
   public:
      TmdbPlaylist(const TmdbPlaylistConfig& config,
                   bool dryRun,
                   std::shared_ptr<warp::ApiManager> apiManager,
                   ServiceLogger logger);
      ~TmdbPlaylist() = default;

      [[nodiscard]] bool IsValid() const;

      void Sync();

   private:
      void ErrorCheckConfig();

      bool valid_{false};

      TmdbPlaylistConfig config_;
      std::shared_ptr<warp::ApiManager> apiManager_;
      ServiceLogger logger_;
      std::string dryRunText_;
   };
} // namespace loomis
