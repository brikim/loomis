#pragma once

#include <filesystem>
#include <string>

namespace warp
{
   class PlexApi;
   class EmbyApi;
} // namespace warp

namespace loomis
{
   struct DeleteWatchedPlexData
   {
      warp::PlexApi* api;
      std::string libraryName;
      std::filesystem::path mediaPath;
   };

   struct DeleteWatchedEmbyData
   {
      warp::EmbyApi* api;
      std::string libraryName;
      std::filesystem::path mediaPath;
   };

   struct DeleteFileInfo
   {
      std::filesystem::path path;
      std::string userName;
      std::string server;
   };
} // namespace loomis
