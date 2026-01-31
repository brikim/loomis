#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace warp
{
   class PlexApi;
   class EmbyApi;
   class TautulliApi;
   class JellystatApi;
}

namespace loomis
{
   struct DeleteWatchedPlexData
   {
      warp::PlexApi* api;
      warp::TautulliApi* trackerApi;
      std::string libraryName;
      std::string mediaPath;
      std::vector<std::string> users;
   };

   struct DeleteWatchedEmbyData
   {
      warp::EmbyApi* api;
      warp::JellystatApi* trackerApi;
      std::string libraryName;
      std::string mediaPath;
      std::vector<std::string> users;
   };

   struct DeleteFileInfo
   {
      std::string id;
      std::filesystem::path path;
      std::string userName;
      std::string server;
   };
}