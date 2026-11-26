#include "tmdb-plex-collection-sync.h"

#include "services/service-utils.h"
#include "services/tmdb-sync/tmdb-container-reconciler.h"

#include <warp/log/log-utils.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace loomis
{
   namespace
   {
      struct CollectionUpdateResult
      {
         std::optional<warp::PlexCollection> collection;
         bool successful{true};
      };

      CollectionUpdateResult ApplyCollectionChanges(
         warp::PlexApi& plexApi,
         warp::PlexCollection& collection,
         const ContainerChanges& changes,
         std::string_view libraryName,
         std::string_view collectionName,
         std::size_t targetSize,
         ServiceLogger logger)
      {
         bool successful = true;
         if (!changes.removeIds.empty() &&
             !plexApi.RemoveCollectionItems(libraryName, collectionName, changes.removeIds))
         {
            logger.LogWarning(
               "{} failed to remove {} from {}",
               plexApi.GetPrettyName(),
               warp::GetTag("item_count", changes.removeIds.size()),
               warp::GetTag("collection", collectionName));
            successful = false;
         }

         if (!changes.addIds.empty() &&
             !plexApi.AddCollectionItems(libraryName, collectionName, changes.addIds))
         {
            logger.LogWarning(
               "{} failed to add {} to {}",
               plexApi.GetPrettyName(),
               warp::GetTag("item_count", changes.addIds.size()),
               warp::GetTag("collection", collectionName));
            successful = false;
         }

         if (!changes.addIds.empty() || !changes.removeIds.empty())
         {
            auto updatedCollection = plexApi.GetCollection(libraryName, collectionName);
            if (!updatedCollection)
            {
               logger.LogWarning(
                  "{} failed to retrieve {} after updating",
                  plexApi.GetPrettyName(),
                  warp::GetTag("collection", collectionName));
               return {std::nullopt, false};
            }
            collection = std::move(*updatedCollection);
         }

         if (!VerifyContainerSize(
                collection.items.size(),
                targetSize,
                plexApi.GetPrettyName(),
                "collection",
                collection.name,
                logger))
         {
            return {std::nullopt, false};
         }

         return {std::move(collection), successful};
      }

      ContainerReorderResult ReorderCollection(
         warp::PlexApi& plexApi,
         const warp::PlexCollection& collection,
         std::span<const std::string_view> targetIds,
         std::string_view libraryName,
         ServiceLogger logger)
      {
         std::vector<ReorderItemTracker> virtualItems;
         virtualItems.reserve(collection.items.size());
         for (const auto& item : collection.items)
            virtualItems.push_back({.id = item.ratingKey, .moveKey = item.ratingKey});

         auto moveFn = [&](const ReorderItemTracker& item, std::size_t /*index*/, std::optional<std::string_view> prevId) {
            return plexApi.MoveCollectionItem(libraryName, collection.name, item.moveKey, prevId);
         };
         return ReorderContainer(
            std::move(virtualItems),
            targetIds,
            moveFn,
            plexApi.GetPrettyName(),
            "collection",
            collection.name,
            logger);
      }

      ContainerSyncResult SyncCollection(
         warp::PlexApi& plexApi,
         std::string_view libraryName,
         std::string_view collectionName,
         std::span<const std::string_view> targetIds,
         ServiceLogger logger,
         bool dryRun)
      {
         ContainerSyncResult result;
         bool collectionCreated = false;
         if (!plexApi.GetCollectionValid(libraryName, collectionName))
         {
            result.creationRequired = true;
            if (dryRun)
               return result;

            if (!plexApi.CreateCollection(libraryName, collectionName, targetIds))
            {
               logger.LogWarning(
                  "{} failed to create collection {}",
                  plexApi.GetPrettyName(),
                  warp::GetTag("collection", collectionName));
               result.successful = false;
               return result;
            }
            collectionCreated = true;
         }

         const auto ensureCustomSort = [&]() {
            bool customSortNeeded = collectionCreated;
            if (!customSortNeeded)
            {
               const auto currentSort = plexApi.GetCollectionSort(libraryName, collectionName);
               customSortNeeded =
                  !currentSort || *currentSort != warp::PlexCollectionSort::Custom;
            }

            if (!customSortNeeded || dryRun)
               return true;

            if (plexApi.SetCollectionSort(
                   libraryName,
                   collectionName,
                   warp::PlexCollectionSort::Custom))
               return true;

            logger.LogWarning(
               "{} failed to set custom ordering for collection {}",
               plexApi.GetPrettyName(),
               warp::GetTag("collection", collectionName));
            result.successful = false;
            return false;
         };
         if (!ensureCustomSort())
            return result;

         auto currentCollection = plexApi.GetCollection(libraryName, collectionName);
         if (!currentCollection)
         {
            logger.LogWarning(
               "{} failed to retrieve collection {}",
               plexApi.GetPrettyName(),
               warp::GetTag("collection", collectionName));
            result.successful = false;
            return result;
         }

         const auto changes = BuildContainerChanges(
            currentCollection->items,
            targetIds,
            // Explicit return types keep these as views; inferred auto would copy the strings.
            [](const warp::PlexCollectionItem& item) -> std::string_view { return item.ratingKey; },
            [](const warp::PlexCollectionItem& item) -> std::string_view { return item.ratingKey; });

         result.added = changes.addIds.size();
         result.removed = changes.removeIds.size();
         if (dryRun)
         {
            result.reordered = WouldRequireReordering(
               currentCollection->items,
               targetIds,
               // Explicit return types keep these as views; inferred auto could copy the strings.
               [](const warp::PlexCollectionItem& item) -> std::string_view { return item.ratingKey; });
            return result;
         }

         auto updateResult = ApplyCollectionChanges(
            plexApi,
            *currentCollection,
            changes,
            libraryName,
            collectionName,
            targetIds.size(),
            logger);

         result.successful = updateResult.successful;

         if (!updateResult.collection)
            return result;

         const auto reorderResult = ReorderCollection(
            plexApi,
            *updateResult.collection,
            targetIds,
            libraryName,
            logger);

         result.reordered = reorderResult.reordered;
         result.successful = result.successful && reorderResult.successful;
         return result;
      }
   } // namespace

   TmdbPlexCollectionSync::TmdbPlexCollectionSync(bool dryRun, ServiceLogger logger)
      : logger_(logger)
      , dryRun_(dryRun)
      , dryRunText_(GetDryRunText(dryRun))
   {
   }

   std::optional<TmdbSyncStats> TmdbPlexCollectionSync::Sync(
      warp::PlexApi& plexApi,
      std::string_view libraryName,
      std::string_view collectionName,
      std::span<const std::string> targetIds)
   {
      const auto uniqueTargetIds = DeduplicateTargetIds(targetIds);
      const auto result = SyncCollection(
         plexApi,
         libraryName,
         collectionName,
         uniqueTargetIds,
         logger_,
         dryRun_);

      return ToSyncStats(
         result,
         "collection",
         collectionName,
         plexApi.GetPrettyName(),
         dryRunText_,
         dryRun_,
         logger_);
   }

   std::optional<TmdbSyncStats> TmdbPlexCollectionSync::Sync(
      warp::PlexApi& plexApi,
      std::string_view libraryName,
      std::string_view collectionName,
      std::span<const std::string_view> targetIds)
   {
      const auto uniqueTargetIds = DeduplicateTargetIds(targetIds);
      const auto result = SyncCollection(
         plexApi,
         libraryName,
         collectionName,
         uniqueTargetIds,
         logger_,
         dryRun_);

      return ToSyncStats(
         result,
         "collection",
         collectionName,
         plexApi.GetPrettyName(),
         dryRunText_,
         dryRun_,
         logger_);
   }
} // namespace loomis
