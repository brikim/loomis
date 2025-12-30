#pragma once

#include <string>
#include <vector>

namespace loomis
{
   struct ServerConfig
   {
      std::string name;
      std::string url;
      std::string apiKey;
      std::string trackerUrl;
      std::string trackerApiKey;
      std::string mediaPath;
   };

   struct AppriseLoggingConfig
   {
      bool enabled{false};
      std::string url;
      std::string key;
      std::string title;
   };

   struct PlaylistPlexCollection
   {
      std::string server;
      std::string library;
      std::string collectionName;
      std::vector<std::string> embyServers;
   };

   struct PlaylistSyncConfig
   {
      bool enabled{false};
      std::string cron;
      uint32_t timeForEmbyUpdateSec{5u};
      uint32_t timeBetweenSyncSec{1u};
      std::vector<PlaylistPlexCollection> plexCollections;
   };

   struct ConfigData
   {
      AppriseLoggingConfig appriseLogging;
      std::vector<ServerConfig> plexServers;
      std::vector<ServerConfig> embyServers;
      PlaylistSyncConfig playlistSync;
   };
}