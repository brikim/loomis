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
      std::filesystem::path mediaPath;
      std::vector<std::string> users;
   };

   struct DeleteWatchedEmbyData
   {
      warp::EmbyApi* api;
      warp::JellystatApi* trackerApi;
      std::string libraryName;
      std::filesystem::path mediaPath;
      std::vector<std::string> users;
   };

   struct DeleteFileInfo
   {
      std::filesystem::path path;
      std::string userName;
      std::string server;
   };
}