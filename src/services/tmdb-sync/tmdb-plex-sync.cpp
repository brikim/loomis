#include "tmdb-plex-sync.h"

#include "services/service-utils.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-plex.h>
#include <warp/log/log-utils.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace loomis
{
   namespace
   {
      std::vector<std::string> ResolvePlexItemIds(
         warp::PlexApi& plexApi,
         std::string_view libraryName,
         const std::vector<int64_t>& tmdbIds)
      {
         std::vector<std::string> tmdbIdStrings;
         tmdbIdStrings.reserve(tmdbIds.size());
         std::unordered_set<std::string> seenTmdbIds;
         seenTmdbIds.reserve(tmdbIds.size());
         for (const auto tmdbId : tmdbIds)
         {
            auto tmdbIdString = std::to_string(tmdbId);
            if (seenTmdbIds.emplace(tmdbIdString).second)
               tmdbIdStrings.emplace_back(std::move(tmdbIdString));
         }

         const auto plexItems = plexApi.GetItemsByTmdbIds(libraryName, tmdbIdStrings);

         std::unordered_map<std::string_view, std::string_view> itemIdsByTmdbId;
         itemIdsByTmdbId.reserve(plexItems.size());
         for (const auto& plexItem : plexItems)
         {
            if (plexItem.libraryName == libraryName && plexItem.tmdbId)
               itemIdsByTmdbId.try_emplace(*plexItem.tmdbId, plexItem.ratingKey);
         }

         std::vector<std::string> itemIds;
         itemIds.reserve(tmdbIds.size());
         for (const auto& tmdbIdString : tmdbIdStrings)
         {
            if (auto matchingItem = itemIdsByTmdbId.find(tmdbIdString);
                matchingItem != itemIdsByTmdbId.end())
               itemIds.emplace_back(matchingItem->second);
         }
         return itemIds;
      }
   } // namespace

   TmdbPlexSync::TmdbPlexSync(
      std::shared_ptr<warp::ApiManager> apiManager,
      bool dryRun,
      ServiceLogger logger)
      : apiManager_(std::move(apiManager))
      , collectionSync_(dryRun, logger)
      , logger_(logger)
      , dryRunText_(GetDryRunText(dryRun))
   {
   }

   std::vector<TmdbSyncLogEntry> TmdbPlexSync::Sync(
      const TmdbPlaylistConfig& config,
      const std::vector<int64_t>& tmdbIds)
   {
      std::vector<TmdbSyncLogEntry> syncLogEntries;
      if (!apiManager_)
      {
         logger_.LogError("{}API manager is unavailable.", dryRunText_);
         return syncLogEntries;
      }

      for (const auto& serverConfig : config.plexServers)
      {
         auto* plexApi = apiManager_->GetPlexApi(serverConfig.name);
         if (!plexApi)
         {
            logger_.LogWarning(
               "{}No Plex API found for {}. Skipping collection sync.",
               dryRunText_,
               warp::GetTag("server_name", serverConfig.name));
            continue;
         }

         if (!plexApi->GetValid())
         {
            logger_.LogWarning(
               "{}{} is unavailable. Skipping collection sync.",
               dryRunText_,
               plexApi->GetPrettyName());
            continue;
         }

         if (!plexApi->GetLibraryId(serverConfig.library))
         {
            logger_.LogWarning(
               "{}No library named {} was found on {}. Skipping collection sync.",
               dryRunText_,
               warp::GetTag("library", serverConfig.library),
               plexApi->GetPrettyName());
            continue;
         }

         auto itemIds = ResolvePlexItemIds(*plexApi, serverConfig.library, tmdbIds);
         if (itemIds.empty())
         {
            logger_.LogWarning(
               "{}No TMDB items from {} were found in {} on {}. Skipping collection sync.",
               dryRunText_,
               warp::GetTag("playlist", config.name),
               warp::GetTag("library", serverConfig.library),
               plexApi->GetPrettyName());
            continue;
         }

         if (auto stats = collectionSync_.Sync(*plexApi, serverConfig.library, config.name, itemIds))
            syncLogEntries.emplace_back(
               TmdbSyncLogEntry{
                  .destination = std::string(plexApi->GetPrettyName()),
                  .stats = *stats});
      }

      return syncLogEntries;
   }
} // namespace loomis
