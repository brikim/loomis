#pragma once

#include <string>
#include <vector>

namespace loomis
{
   struct JsonServerResponse
   {
      std::string ServerName;
   };

   struct JsonEmbyItem
   {
      std::string Id;
      std::string Type;
      std::string Name;
      std::string Path;
      std::string SeriesName;
      uint32_t ParentIndexNumber{0};
      uint32_t IndexNumber{0};
      uint64_t RunTimeTicks{0};
   };

   struct JsonEmbyItemsResponse
   {
      std::vector<JsonEmbyItem> Items;
   };

   struct PathRebuildItem
   {
      std::string Id;
      std::string Path;
      std::string DateModified;
   };

   struct PathRebuildItems
   {
      std::vector<PathRebuildItem> Items;
   };

   struct JsonEmbyPlaylistItem
   {
      std::string Id;
      std::string Name;
      std::string PlaylistItemId;
   };

   struct JsonEmbyPlaylistItemsResponse
   {
      std::vector<JsonEmbyPlaylistItem> Items;
   };

   struct JsonEmbyUser
   {
      std::string Name;
   };

   struct JsonEmbyLibrary
   {
      std::string Name;
      std::string Id;
   };
}