#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace loomis
{
   struct ServerConfig
   {
      std::string server_name;
      std::optional<std::string> tracearr_server_name;
      std::string url;
      std::string api_key;
      std::string tracker_url;
      std::string tracker_api_key;
      std::filesystem::path media_path;
   };

   struct AppriseLoggingConfig
   {
      bool enabled{false};
      std::string url;
      std::string key;
      std::string message_title;
   };

   struct GotifyLoggingConfig
   {
      bool enabled{false};
      std::string url;
      std::string key;
      std::string message_title;
      int32_t priority{5};
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
      std::string userName;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "server", &ServerUser::server,
            "user_name", &ServerUser::userName
         );
      };
   };

   struct ServerPlexUser
   {
      std::string server;
      std::string userName;
      std::optional<std::string> token;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "server", &ServerPlexUser::server,
            "user_name", &ServerPlexUser::userName,
            "token", &ServerPlexUser::token
         );
      };
   };

   struct ExtensionConfig
   {
      std::string extension;

      static constexpr auto value = glz::object(
            "extension", &ExtensionConfig::extension
      );
   };

   struct ServerTracearrUser
   {
      std::string userName;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "user_name", &ServerTracearrUser::userName
         );
      };
   };

   struct UserSyncConfig
   {
      ServerTracearrUser tracearr;
      std::vector<ServerPlexUser> plex;
      std::vector<ServerUser> emby;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "tracearr", &UserSyncConfig::tracearr,
            "plex", &UserSyncConfig::plex,
            "emby", &UserSyncConfig::emby
         );
      };
   };

   struct StateSyncConfig
   {
      bool enabled{false};
      bool dryRun{false};
      std::string cron;
      std::vector<UserSyncConfig> users;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "enabled", &StateSyncConfig::enabled,
            "dry_run", &StateSyncConfig::dryRun,
            "cron", &StateSyncConfig::cron,
            "users", &StateSyncConfig::users
         );
      };
   };

   struct ServerLibraryConfig
   {
      std::string server;
      std::string library;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "server", &ServerLibraryConfig::server,
            "library_name", &ServerLibraryConfig::library
         );
      };
   };

   struct FscIgnoreItem
   {
      std::string item;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "ignore", &FscIgnoreItem::item
         );
      };
   };

   struct FscEmptyFolderPathConfig
   {
      std::filesystem::path path;
      std::vector<ServerLibraryConfig> plex;
      std::vector<ServerLibraryConfig> emby;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "path", &FscEmptyFolderPathConfig::path,
            "plex", &FscEmptyFolderPathConfig::plex,
            "emby", &FscEmptyFolderPathConfig::emby
         );
      };
   };

   struct FscEmptyFolderDeleteConfig
   {
      std::vector<FscEmptyFolderPathConfig> paths;
      std::vector<FscIgnoreItem> ignoreFolders;
      std::vector<FscIgnoreItem> ignoreFileEmptyCheck;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "paths", &FscEmptyFolderDeleteConfig::paths,
            "ignore_folder_in_empty_check", &FscEmptyFolderDeleteConfig::ignoreFolders,
            "ignore_file_in_empty_check", &FscEmptyFolderDeleteConfig::ignoreFileEmptyCheck
         );
      };
   };

   struct FscDeleteFilePathConfig
   {
      std::filesystem::path path;
      int64_t deleteAgeHours{std::numeric_limits<int64_t>::max()};
      std::vector<ExtensionConfig> validExtensions;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "path", &FscDeleteFilePathConfig::path,
            "delete_age_hours", &FscDeleteFilePathConfig::deleteAgeHours,
            "valid_extensions_to_delete", &FscDeleteFilePathConfig::validExtensions
         );
      };
   };

   struct FscDeleteFilesByAgeConfig
   {
      std::vector<FscDeleteFilePathConfig> paths;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "paths", &FscDeleteFilesByAgeConfig::paths
         );
      };
   };

   struct FileSystemCleanupConfig
   {
      bool enabled{false};
      bool dryRun{false};
      std::string cron;
      FscEmptyFolderDeleteConfig emptyFolderDeleteConfig;
      FscDeleteFilesByAgeConfig deleteFilesByAgeConfig;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "enabled", &FileSystemCleanupConfig::enabled,
            "dry_run", &FileSystemCleanupConfig::dryRun,
            "cron", &FileSystemCleanupConfig::cron,
            "empty_folder_delete", &FileSystemCleanupConfig::emptyFolderDeleteConfig,
            "delete_files_by_age", &FileSystemCleanupConfig::deleteFilesByAgeConfig
         );
      };
   };

   struct DvrMaintainerLibraryActionInfo
   {
      std::string name;
      std::string action;
   };

   struct DvrMaintainerLibrary
   {
      std::vector<ServerLibraryConfig> plex;
      std::vector<ServerLibraryConfig> emby;
      std::filesystem::path path;
      std::vector<DvrMaintainerLibraryActionInfo> actions;
      std::vector<ExtensionConfig> extensionsToDelete;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "plex", &DvrMaintainerLibrary::plex,
            "emby", &DvrMaintainerLibrary::emby,
            "path", &DvrMaintainerLibrary::path,
            "actions", &DvrMaintainerLibrary::actions,
            "extensions_to_delete", &DvrMaintainerLibrary::extensionsToDelete
         );
      };
   };

   struct DvrMaintainerConfig
   {
      bool enabled{false};
      bool dryRun{false};
      std::string cron;
      std::vector<DvrMaintainerLibrary> libraries;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "enabled", &DvrMaintainerConfig::enabled,
            "dry_run", &DvrMaintainerConfig::dryRun,
            "cron", &DvrMaintainerConfig::cron,
            "libraries", &DvrMaintainerConfig::libraries
         );
      };
   };

   struct DeleteWatchedUserConfig
   {
      std::string name;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "name", &DeleteWatchedUserConfig::name
         );
      };
   };

   struct DeleteWatchedServerConfig
   {
      std::string server;
      std::string library;
      std::vector<DeleteWatchedUserConfig> users;
      std::filesystem::path mediaPath;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "server", &DeleteWatchedServerConfig::server,
            "library_name", &DeleteWatchedServerConfig::library,
            "users", &DeleteWatchedServerConfig::users,
            "media_path", &DeleteWatchedServerConfig::mediaPath
         );
      };
   };

   struct DeleteWatchedLibraryConfig
   {
      std::filesystem::path containerPath;
      std::vector<DeleteWatchedServerConfig> plex;
      std::vector<DeleteWatchedServerConfig> emby;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "container_path", &DeleteWatchedLibraryConfig::containerPath,
            "plex", &DeleteWatchedLibraryConfig::plex,
            "emby", &DeleteWatchedLibraryConfig::emby
         );
      };
   };

   struct DeleteWatchedConfig
   {
      bool enabled{false};
      bool dryRun{false};
      std::string cron;
      int32_t deleteTimeHours{24};
      std::vector<DeleteWatchedLibraryConfig> libraries;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "enabled", &DeleteWatchedConfig::enabled,
            "dry_run", &DeleteWatchedConfig::dryRun,
            "cron", &DeleteWatchedConfig::cron,
            "delete_time_hours", &DeleteWatchedConfig::deleteTimeHours,
            "libraries", &DeleteWatchedConfig::libraries
         );
      };
   };

   struct EmbyTidyLibraryConfig
   {
      std::string name;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "library", &EmbyTidyLibraryConfig::name
         );
      };
   };

   struct EmbyTidyServerConfig
   {
      std::string name;
      bool cleanUpNonLocalBackdrops;
      std::filesystem::path localBackdropContainerStartPath;
      std::vector<EmbyTidyLibraryConfig> libraries;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "server", &EmbyTidyServerConfig::name,
            "clean_up_non_local_backdrops", &EmbyTidyServerConfig::cleanUpNonLocalBackdrops,
            "local_backdrop_container_start_path", &EmbyTidyServerConfig::localBackdropContainerStartPath,
            "libraries", &EmbyTidyServerConfig::libraries
         );
      };
   };

   struct EmbyTidyConfig
   {
      bool enabled{false};
      bool dryRun{false};
      std::string cron;
      std::vector<EmbyTidyServerConfig> servers;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "enabled", &EmbyTidyConfig::enabled,
            "dry_run", &EmbyTidyConfig::dryRun,
            "cron", &EmbyTidyConfig::cron,
            "servers", &EmbyTidyConfig::servers
         );
      };
   };

   struct ConfigServers
   {
      std::vector<ServerConfig> servers;
   };

   struct TracearrServer
   {
      std::optional<std::string> server_name;
      std::optional<std::string> url;
      std::optional<std::string> api_key;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "server_name", &TracearrServer::server_name,
            "url", &TracearrServer::url,
            "api_key", &TracearrServer::api_key
         );
      };
   };

   struct ConfigData
   {
      ConfigServers plex;
      ConfigServers emby;
      TracearrServer tracearr;
      AppriseLoggingConfig appriseLogging;
      GotifyLoggingConfig gotifyLogging;
      PlaylistSyncConfig playlistSync;
      FileSystemCleanupConfig fileSystemCleanup;
      DvrMaintainerConfig dvrMaintainer;
      DeleteWatchedConfig deleteWatched;
      EmbyTidyConfig embyTidy;
      StateSyncConfig stateSync;

      struct glaze
      {
         static constexpr auto value = glz::object(
            "plex", &ConfigData::plex,
            "emby", &ConfigData::emby,
            "tracearr", &ConfigData::tracearr,
            "apprise_logging", &ConfigData::appriseLogging,
            "gotify_logging", &ConfigData::gotifyLogging,
            "playlist_sync", &ConfigData::playlistSync,
            "filesystem_cleanup", &ConfigData::fileSystemCleanup,
            "dvr_maintainer", &ConfigData::dvrMaintainer,
            "delete_watched", &ConfigData::deleteWatched,
            "emby_tidy", &ConfigData::embyTidy,
            "state_sync", &ConfigData::stateSync
         );
      };
   };
}