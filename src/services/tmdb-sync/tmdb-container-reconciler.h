#pragma once

#include "services/service-logger.h"
#include "services/tmdb-sync/tmdb-sync-result.h"

#include <warp/log/log-utils.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace loomis
{
   struct ContainerSyncResult
   {
      std::size_t added{0};
      std::size_t removed{0};
      bool reordered{false};
      bool creationRequired{false};
      bool successful{true};
   };

   struct ContainerChanges
   {
      std::vector<std::string> addIds;
      std::vector<std::string> removeIds;
   };

   struct ContainerReorderResult
   {
      bool reordered{false};
      bool successful{true};
   };

   struct ReorderItemTracker
   {
      std::string_view id;
      std::string_view moveKey;
   };

   [[nodiscard]] inline std::vector<std::string> DeduplicateTargetIds(std::span<const std::string> targetIds)
   {
      std::vector<std::string> uniqueIds;
      uniqueIds.reserve(targetIds.size());
      std::unordered_set<std::string_view> seen;
      seen.reserve(targetIds.size());
      for (const auto& id : targetIds)
      {
         if (seen.emplace(id).second)
            uniqueIds.emplace_back(id);
      }
      return uniqueIds;
   }

   template <typename ItemRange, typename GetIdFn, typename GetRemoveIdFn>
   [[nodiscard]] ContainerChanges BuildContainerChanges(
      const ItemRange& currentItems,
      std::span<const std::string> targetIds,
      GetIdFn&& getId,
      GetRemoveIdFn&& getRemoveId)
   {
      std::unordered_set<std::string_view> targetIdSet;
      targetIdSet.reserve(targetIds.size());
      for (const auto& id : targetIds)
         targetIdSet.emplace(id);

      std::unordered_set<std::string_view> currentItemIds;
      currentItemIds.reserve(std::ranges::size(currentItems));
      std::unordered_set<std::string_view> seenRemoveIds;

      ContainerChanges changes;
      changes.removeIds.reserve(std::ranges::size(currentItems));

      for (const auto& item : currentItems)
      {
         const auto id = getId(item);
         const bool firstOccurrence = currentItemIds.emplace(id).second;
         if (!targetIdSet.contains(id) || !firstOccurrence)
         {
            const auto removeId = getRemoveId(item);
            if (seenRemoveIds.emplace(removeId).second)
               changes.removeIds.emplace_back(std::string(removeId));
         }
      }

      changes.addIds.reserve(targetIds.size());
      for (const auto& targetId : targetIds)
      {
         if (!currentItemIds.contains(targetId))
            changes.addIds.emplace_back(targetId);
      }

      return changes;
   }

   template <typename ItemRange, typename GetIdFn>
   [[nodiscard]] bool WouldRequireReordering(
      const ItemRange& currentItems,
      std::span<const std::string> targetIds,
      GetIdFn&& getId)
   {
      std::unordered_set<std::string_view> currentItemIds;
      currentItemIds.reserve(std::ranges::size(currentItems));
      for (const auto& item : currentItems)
         currentItemIds.emplace(getId(item));

      std::unordered_set<std::string_view> targetIdSet;
      targetIdSet.reserve(targetIds.size());
      for (const auto& id : targetIds)
         targetIdSet.emplace(id);

      std::vector<std::string_view> plannedOrder;
      plannedOrder.reserve(targetIds.size());
      std::unordered_set<std::string_view> seenPlanned;
      for (const auto& item : currentItems)
      {
         const auto id = getId(item);
         if (targetIdSet.contains(id) && seenPlanned.emplace(id).second)
            plannedOrder.emplace_back(id);
      }
      for (const auto& id : targetIds)
      {
         if (!currentItemIds.contains(id))
            plannedOrder.emplace_back(id);
      }

      return plannedOrder.size() != targetIds.size() ||
         !std::ranges::equal(plannedOrder, targetIds);
   }

   template <typename MoveFn>
   [[nodiscard]] ContainerReorderResult ReorderContainer(
      std::vector<ReorderItemTracker> virtualItems,
      std::span<const std::string> targetIds,
      MoveFn&& moveFn,
      std::string_view serverName,
      std::string_view containerTag,
      std::string_view containerName,
      ServiceLogger logger)
   {
      ContainerReorderResult result;
      for (std::size_t index = 0; index < targetIds.size(); ++index)
      {
         if (index >= virtualItems.size())
            break;

         if (virtualItems[index].id == targetIds[index])
            continue;

         auto itemToMove = std::ranges::find_if(
            virtualItems | std::views::drop(index),
            [&](const auto& item) { return item.id == targetIds[index]; });

         if (itemToMove == virtualItems.end())
            continue;

         const auto prevId = (index == 0)
            ? std::optional<std::string_view>{}
            : std::optional<std::string_view>{targetIds[index - 1]};

         if (moveFn(*itemToMove, index, prevId))
         {
            const auto movedItem = *itemToMove;
            virtualItems.erase(itemToMove);
            virtualItems.insert(virtualItems.begin() + static_cast<std::ptrdiff_t>(index), movedItem);
            result.reordered = true;
         }
         else
         {
            logger.LogWarning(
               "{} failed to reorder {}",
               serverName,
               warp::GetTag(containerTag, containerName));
            result.successful = false;
         }
      }
      return result;
   }

   [[nodiscard]] inline bool VerifyContainerSize(
      std::size_t actualSize,
      std::size_t expectedSize,
      std::string_view serverName,
      std::string_view containerTag,
      std::string_view containerName,
      ServiceLogger logger)
   {
      if (actualSize != expectedSize)
      {
         logger.LogWarning(
            "{} {} update failed. {} should be {} but is {}",
            serverName,
            containerTag,
            warp::GetTag(containerTag, containerName),
            warp::GetTag("length", expectedSize),
            warp::GetTag("reported_length", actualSize));
         return false;
      }
      return true;
   }

   [[nodiscard]] inline std::optional<TmdbSyncStats> ToSyncStats(
      const ContainerSyncResult& result,
      std::string_view containerTag,
      std::string_view containerName,
      std::string_view serverPrettyName,
      std::string_view dryRunText,
      bool dryRun,
      ServiceLogger logger)
   {
      if (result.creationRequired && (result.successful || dryRun))
      {
         logger.LogInfo(
            "{}Creating TMDB {} on {}",
            dryRunText,
            warp::GetTag(containerTag, containerName),
            serverPrettyName);
         return std::nullopt;
      }
      else if (result.successful && (result.added > 0 || result.removed > 0 || result.reordered))
      {
         return TmdbSyncStats{
            .added = result.added,
            .removed = result.removed,
            .reordered = result.reordered};
      }

      return std::nullopt;
   }
} // namespace loomis
