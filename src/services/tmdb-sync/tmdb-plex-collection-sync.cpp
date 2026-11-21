#include "tmdb-plex-collection-sync.h"

#include <warp/log/log-utils.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace loomis
{
   namespace
   {
      struct CollectionSyncResult
      {
         std::size_t added{0};
         std::size_t removed{0};
         bool reordered{false};
         bool creationRequired{false};
         bool successful{true};
      };

      struct CollectionChanges
      {
         std::vector<std::string> addIds;
         std::vector<std::string> removeIds;
      };

      CollectionChanges BuildCollectionChanges(
         const warp::PlexCollection& collection,
         const std::vector<std::string>& targetIds)
      {
         std::unordered_set<std::string_view> targetIdSet;
         targetIdSet.reserve(targetIds.size());
         for (const auto& id : targetIds)
            targetIdSet.emplace(id);

         std::unordered_set<std::string_view> currentItemIds;
         currentItemIds.reserve(collection.items.size());
         CollectionChanges changes;
         changes.removeIds.reserve(collection.items.size());
         for (const auto& item : collection.items)
         {
            const bool firstOccurrence = currentItemIds.emplace(item.ratingKey).second;
            if (!targetIdSet.contains(item.ratingKey) || !firstOccurrence)
               changes.removeIds.emplace_back(item.ratingKey);
         }

         changes.addIds.reserve(targetIds.size());
         for (const auto& targetId : targetIds)
         {
            if (!currentItemIds.contains(targetId))
               changes.addIds.emplace_back(targetId);
         }

         return changes;
      }

      bool WouldRequireReordering(
         const warp::PlexCollection& collection,
         const std::vector<std::string>& targetIds)
      {
         std::unordered_set<std::string_view> currentItemIds;
         currentItemIds.reserve(collection.items.size());
         for (const auto& item : collection.items)
            currentItemIds.emplace(item.ratingKey);

         std::unordered_set<std::string_view> targetIdSet;
         targetIdSet.reserve(targetIds.size());
         for (const auto& id : targetIds)
            targetIdSet.emplace(id);

         std::vector<std::string_view> plannedOrder;
         plannedOrder.reserve(targetIds.size());
         for (const auto& item : collection.items)
         {
            if (targetIdSet.contains(item.ratingKey))
               plannedOrder.emplace_back(item.ratingKey);
         }
         for (const auto& id : targetIds)
         {
            if (!currentItemIds.contains(id))
               plannedOrder.emplace_back(id);
         }

         return plannedOrder.size() != targetIds.size() ||
            !std::ranges::equal(plannedOrder, targetIds);
      }

      struct CollectionUpdateResult
      {
         std::optional<warp::PlexCollection> collection;
         bool successful{true};
      };

      CollectionUpdateResult ApplyCollectionChanges(
         warp::PlexApi& plexApi,
         warp::PlexCollection collection,
         const CollectionChanges& changes,
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
               "{} failed to remove {} from collection {}",
               plexApi.GetPrettyName(),
               warp::GetTag("item_count", changes.removeIds.size()),
               warp::GetTag("collection", collectionName));
            successful = false;
         }

         if (!changes.addIds.empty() &&
             !plexApi.AddCollectionItems(libraryName, collectionName, changes.addIds))
         {
            logger.LogWarning(
               "{} failed to add {} to collection {}",
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
                  "{} failed to retrieve collection {} after updating",
                  plexApi.GetPrettyName(),
                  warp::GetTag("collection", collectionName));
               return {std::nullopt, false};
            }
            collection = std::move(*updatedCollection);
         }

         if (collection.items.size() != targetSize)
         {
            logger.LogWarning(
               "{} collection update failed. {} should be {} but is {}",
               plexApi.GetPrettyName(),
               warp::GetTag("collection", collection.name),
               warp::GetTag("length", targetSize),
               warp::GetTag("reported_length", collection.items.size()));
            return {std::nullopt, false};
         }

         return {std::move(collection), successful};
      }

      struct CollectionReorderResult
      {
         bool reordered{false};
         bool successful{true};
      };

      CollectionReorderResult ReorderCollection(
         warp::PlexApi& plexApi,
         const warp::PlexCollection& collection,
         const std::vector<std::string>& targetIds,
         std::string_view libraryName,
         ServiceLogger logger)
      {
         CollectionReorderResult result;
         std::vector<std::string_view> virtualItems;
         virtualItems.reserve(collection.items.size());
         for (const auto& item : collection.items)
            virtualItems.emplace_back(item.ratingKey);

         for (std::size_t index = 0; index < targetIds.size(); ++index)
         {
            if (virtualItems[index] == targetIds[index])
               continue;

            auto itemToMove = std::ranges::find(
               virtualItems | std::views::drop(index),
               targetIds[index]);
            if (itemToMove == virtualItems.end())
               continue;

            const auto afterRatingKey = index == 0
               ? std::optional<std::string_view>{}
               : std::optional<std::string_view>{targetIds[index - 1]};
            if (plexApi.MoveCollectionItem(
                   libraryName,
                   collection.name,
                   *itemToMove,
                   afterRatingKey))
            {
               const auto movedItem = *itemToMove;
               virtualItems.erase(itemToMove);
               virtualItems.insert(
                  virtualItems.begin() + static_cast<std::ptrdiff_t>(index),
                  movedItem);
               result.reordered = true;
            }
            else
            {
               logger.LogWarning(
                  "{} failed to reorder collection {}",
                  plexApi.GetPrettyName(),
                  warp::GetTag("collection", collection.name));
               result.successful = false;
            }
         }

         return result;
      }

      CollectionSyncResult SyncCollection(
         warp::PlexApi& plexApi,
         std::string_view libraryName,
         std::string_view collectionName,
         const std::vector<std::string>& targetIds,
         ServiceLogger logger,
         bool dryRun)
      {
         CollectionSyncResult result;
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

         const auto changes = BuildCollectionChanges(*currentCollection, targetIds);
         result.added = changes.addIds.size();
         result.removed = changes.removeIds.size();
         if (dryRun)
         {
            result.reordered = WouldRequireReordering(*currentCollection, targetIds);
            return result;
         }

         auto updateResult = ApplyCollectionChanges(
            plexApi,
            std::move(*currentCollection),
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
      , dryRunText_(dryRun ? "[DRY RUN] " : "")
   {
   }

   void TmdbPlexCollectionSync::Sync(
      warp::PlexApi& plexApi,
      std::string_view libraryName,
      std::string_view collectionName,
      const std::vector<std::string>& targetIds)
   {
      const auto result = SyncCollection(
         plexApi,
         libraryName,
         collectionName,
         targetIds,
         logger_,
         dryRun_);
      if (result.creationRequired && (result.successful || dryRun_))
      {
         logger_.LogInfo(
            "{}Creating TMDB {} on {}",
            dryRunText_,
            warp::GetTag("collection", collectionName),
            plexApi.GetPrettyName());
      }
      else if (result.successful &&
               (result.added > 0 || result.removed > 0 || result.reordered))
      {
         logger_.LogInfo(
            "{}Syncing TMDB {} to {} {} {} {}",
            dryRunText_,
            warp::GetTag("collection", collectionName),
            plexApi.GetPrettyName(),
            warp::GetTag("added", result.added),
            warp::GetTag("removed", result.removed),
            warp::GetTag("reordered", result.reordered ? "true" : "false"));
      }
   }
} // namespace loomis
