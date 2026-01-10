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

   struct FolderCleanupServerConfig
   {
      std::string server;
      std::string libraryName;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "server", &FolderCleanupServerConfig::server,
            "library_name", &FolderCleanupServerConfig::libraryName
         );
      };
   };

   struct FolderCleanupIgnoreItem
   {
      std::string item;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "ignore", &FolderCleanupIgnoreItem::item
         );
      };
   };

   struct FolderCleanupPathToCheck
   {
      std::string path;
      std::vector <FolderCleanupServerConfig> plex;
      std::vector <FolderCleanupServerConfig> emby;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "path", &FolderCleanupPathToCheck::path,
            "plex", &FolderCleanupPathToCheck::plex,
            "emby", &FolderCleanupPathToCheck::emby
         );
      };
   };

   struct FolderCleanupConfig
   {
      bool enabled{false};
      bool dryRun{false};
      std::string cron;
      std::vector<FolderCleanupPathToCheck> pathsToCheck;
      std::vector<FolderCleanupIgnoreItem> ignoreFolders;
      std::vector<FolderCleanupIgnoreItem> ignoreFileEmptyCheck;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "enabled", &FolderCleanupConfig::enabled,
            "dry_run", &FolderCleanupConfig::dryRun,
            "cron", &FolderCleanupConfig::cron,
            "paths_to_check", &FolderCleanupConfig::pathsToCheck,
            "ignore_folder_in_empty_check", &FolderCleanupConfig::ignoreFolders,
            "ignore_file_in_empty_check", &FolderCleanupConfig::ignoreFileEmptyCheck
         );
      };
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
      FolderCleanupConfig folder_cleanup;
   };
}