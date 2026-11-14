#include "tmdb-playlist.h"

#include <array>
#include <numeric>
#include <span>

namespace loomis
{
   namespace
   {
      constexpr std::string_view TYPE_TRENDING{"trending"};
      constexpr std::array<std::string_view, 5> VALID_TYPES{TYPE_TRENDING, "popular", "top_rated", "now_playing", "upcoming"};
      constexpr std::array<std::string_view, 2> VALID_TIME_WINDOWS{"day", "week"};

      inline std::string BuildCommaSeparatedList(std::span<const std::string_view> list)
      {
         if (list.empty())
            return "";

         return std::accumulate(std::next(list.begin()), list.end(), std::string(list[0]), [](std::string a, std::string_view b) {
            a.reserve(a.size() + 1 + b.size());
            a.push_back(',');
            a.append(b);
            return a;
         });
      }
   } // namespace

   TmdbPlaylist::TmdbPlaylist(const TmdbPlaylistConfig& config,
                              bool dryRun,
                              std::shared_ptr<warp::ApiManager> apiManager,
                              ServiceLogger logger)
      : config_(config)
      , apiManager_(std::move(apiManager))
      , logger_(logger)
      , dryRunText_(dryRun ? "[DRY RUN] " : "")
   {
      ErrorCheckConfig();
   }

   void TmdbPlaylist::ErrorCheckConfig()
   {
      if (config_.embyServers.empty() && config_.plexServers.empty())
      {
         logger_.LogError("{}No servers configured. At least one Plex or Emby server is required.", dryRunText_);
         return;
      }

      if (VALID_TYPES.end() == std::ranges::find(VALID_TYPES, config_.type))
      {
         logger_.LogError("{}Invalid playlist type: {}. Valid types are: {}",
                          dryRunText_,
                          config_.type,
                          BuildCommaSeparatedList(VALID_TYPES));
         return;
      }

      if (config_.type == TYPE_TRENDING)
      {
         if (!config_.timeWindow.has_value())
         {
            logger_.LogError("{}No time window. Required for {}. Valid time windows are: {}",
                             dryRunText_,
                             config_.type,
                             BuildCommaSeparatedList(VALID_TIME_WINDOWS));
            return;
         }
         else if (VALID_TIME_WINDOWS.end() == std::ranges::find(VALID_TIME_WINDOWS, config_.timeWindow.value()))
         {
            logger_.LogError("{}Invalid time window: {}. Required for {}. Valid time windows are: {}",
                             dryRunText_,
                             config_.timeWindow.value(),
                             config_.type,
                             BuildCommaSeparatedList(VALID_TIME_WINDOWS));
            return;
         }
      }

      valid_ = true;
   }

   bool TmdbPlaylist::IsValid() const
   {
      return valid_;
   }

   void TmdbPlaylist::Sync()
   {
   }
} // namespace loomis
