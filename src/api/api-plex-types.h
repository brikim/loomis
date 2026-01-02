#pragma once

#include <string>
#include <vector>

namespace loomis
{
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
}