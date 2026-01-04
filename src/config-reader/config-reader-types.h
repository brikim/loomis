#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace loomis
{
   struct ServerConfig
   {
      std::string server_name;
      std::string url;
      std::string api_key;
      std::string tracker_url;
      std::string tracker_api_key;
      std::string media_path;
   };

   struct AppriseLoggingConfig
   {
      bool enabled{false};
      std::string url;
      std::string key;
      std::string message_title;
   };

   struct PlaylistEmbyServers
   {
      std::string server;
   };

   struct PlaylistPlexCollection
   {
      std::string server;
      std::string library;
      std::string collection_name;
      std::vector<PlaylistEmbyServers> target_emby_servers;
   };

   struct PlaylistSyncConfig
   {
      bool enabled{false};
      std::string cron;
      uint32_t time_for_emby_to_update_seconds{5u};
      uint32_t time_between_syncs_seconds{1u};
      std::vector<PlaylistPlexCollection> plex_collection_sync;
   };

   struct ServerUser
   {
      std::string server;
      std::string user_name;
      bool can_sync{false};
   };

   struct UserSyncConfig
   {
      std::vector<ServerUser> plex;
      std::vector<ServerUser> emby;
   };

   struct WatchStateSyncConfig
   {
      bool enabled{false};
      std::string cron;
      std::vector<UserSyncConfig> users;
   };

   struct ConfigServers
   {
      std::vector<ServerConfig> servers;
   };

   struct ConfigData
   {
      ConfigServers plex;
      ConfigServers emby;
      AppriseLoggingConfig apprise_logging;
      PlaylistSyncConfig playlist_sync;
      WatchStateSyncConfig watch_state_sync;
   };
}