#pragma once

#include <string>
#include <vector>

namespace loomis
{
   enum class PlexSearchTypes
   {
      movie = 1,
      show = 2,
      season = 3,
      episode = 4,
      trailer = 5,
      comic = 6,
      person = 7,
      artist = 8,
      album = 9,
      track = 10,
      picture = 11,
      clip = 12,
      photo = 13,
      photoalbum = 14,
      playlist = 15,
      playlistFolder = 16,
      collection = 18,
      optimizedVersion = 42,
      userPlaylistItem = 1001,
   };

   struct PlexCollectionItem
   {
      std::string title;
      std::vector<std::string> paths;
   };

   struct PlexCollection
   {
      std::string name;
      std::vector<PlexCollectionItem> items;
   };

   struct PlexSearchResult
   {
      std::string ratingKey;
      std::string path;
      std::string title;
      std::string libraryName;
      int64_t durationMs{0};
   };

   struct PlexSearchResults
   {
      std::vector<PlexSearchResult> items;
   };
}