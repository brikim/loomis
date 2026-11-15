#include "tmdb-emby-sync.h"

#include "services/tmdb-sync/tmdb-string-utils.h"

#include <warp/api/api-emby.h>
#include <warp/api/api-manager.h>
#include <warp/log/log-utils.h>

#include <string>
#include <unordered_map>
#include <utility>

namespace loomis
{
   namespace
   {
      std::vector<std::string> ResolveEmbyItemIds(
         warp::EmbyApi& embyApi,
         const std::vector<int64_t>& tmdbIds)
      {
         std::vector<std::string> tmdbIdStrings;
         tmdbIdStrings.reserve(tmdbIds.size());
         for (const auto tmdbId : tmdbIds)
            tmdbIdStrings.emplace_back(std::to_string(tmdbId));

         const auto embyItems = embyApi.GetItemsByTmdbIds(tmdbIdStrings);

         std::unordered_map<std::string, std::string> movieItemIds;
         for (const auto& embyItem : embyItems)
         {
            if (embyItem.type == "Movie" && embyItem.tmdbId)
               movieItemIds.try_emplace(*embyItem.tmdbId, embyItem.id);
         }

         std::vector<std::string> itemIds;
         itemIds.reserve(tmdbIds.size());
         for (const auto tmdbId : tmdbIds)
         {
            if (auto matchingItem = movieItemIds.find(std::to_string(tmdbId));
                matchingItem != movieItemIds.end())
               itemIds.emplace_back(matchingItem->second);
         }
         return itemIds;
      }
   } // namespace

   TmdbEmbySync::TmdbEmbySync(
      std::shared_ptr<warp::ApiManager> apiManager,
      bool dryRun,
      ServiceLogger logger)
      : apiManager_(std::move(apiManager))
      , playlistSync_(dryRun, logger)
      , logger_(logger)
      , dryRunText_(dryRun ? "[DRY RUN] " : "")
   {
   }

   void TmdbEmbySync::Sync(
      const TmdbPlaylistConfig& config,
      const std::vector<int64_t>& tmdbIds)
   {
      if (!apiManager_)
      {
         logger_.LogError("{}API manager is unavailable.", dryRunText_);
         return;
      }

      for (const auto& serverConfig : config.embyServers)
      {
         auto* embyApi = apiManager_->GetEmbyApi(serverConfig.name);
         if (!embyApi)
         {
            logger_.LogWarning(
               "{}No Emby API found for {}. Skipping playlist sync.",
               dryRunText_,
               warp::GetTag("server_name", serverConfig.name));
            continue;
         }

         if (!embyApi->GetValid())
         {
            logger_.LogWarning(
               "{}{} is unavailable. Skipping playlist sync.",
               dryRunText_,
               embyApi->GetPrettyName());
            continue;
         }

         auto itemIds = ResolveEmbyItemIds(*embyApi, tmdbIds);
         if (itemIds.empty())
         {
            logger_.LogWarning(
               "{}No TMDB items from {} were found on {}. Skipping playlist sync.",
               dryRunText_,
               warp::GetTag("playlist", config.name),
               embyApi->GetPrettyName());
            continue;
         }

         playlistSync_.Sync(*embyApi, config.name, itemIds);
      }
   }
} // namespace loomis
