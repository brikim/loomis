#pragma once

#include "services/tmdb-sync/tmdb-media-item.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <glaze/glaze.hpp>
#include <limits>
#include <optional>
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
      std::vector<PlaylistPlexCollection> plex_collection_sync;
   };

   struct ExtensionConfig
   {
      std::string extension;

      // clang-format off
      static constexpr auto value = glz::object(
            "extension", &ExtensionConfig::extension
      );
      // clang-format on
   };

   struct StateSyncConfig
   {
      bool enabled{false};
      bool dryRun{false};
      std::string cron;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "enabled", &StateSyncConfig::enabled,
            "dry_run", &StateSyncConfig::dryRun,
            "cron", &StateSyncConfig::cron
         );
         // clang-format on
      };
   };

   struct TmdbPlaylistPlexServerConfig
   {
      std::string name;
      std::string library;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "server", &TmdbPlaylistPlexServerConfig::name,
            "library_name", &TmdbPlaylistPlexServerConfig::library
         );
         // clang-format on
      };
   };

   struct TmdbPlaylistEmbyServerConfig
   {
      std::string name;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "server", &TmdbPlaylistEmbyServerConfig::name
         );
         // clang-format on
      };
   };

   struct TmdbPlaylistGenreConfig
   {
      std::string withGenres;
      std::string withoutGenres;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "with", &TmdbPlaylistGenreConfig::withGenres,
            "without", &TmdbPlaylistGenreConfig::withoutGenres
         );
         // clang-format on
      };
   };

   struct TmdbPlaylistConfig
   {
      std::string name;
      std::string mediaType{TMDB_MEDIA_TYPE_MOVIE};
      std::string type;
      std::optional<std::string> timeWindow;
      std::optional<TmdbPlaylistGenreConfig> genre;
      // Optional here so playlist validation can reject a missing config value.
      std::optional<std::size_t> maxItems;
      std::vector<TmdbPlaylistPlexServerConfig> plexServers;
      std::vector<TmdbPlaylistEmbyServerConfig> embyServers;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "name", &TmdbPlaylistConfig::name,
            "media_type", &TmdbPlaylistConfig::mediaType,
            "type", &TmdbPlaylistConfig::type,
            "time_window", &TmdbPlaylistConfig::timeWindow,
            "genre", &TmdbPlaylistConfig::genre,
            "max_items", &TmdbPlaylistConfig::maxItems,
            "plex", &TmdbPlaylistConfig::plexServers,
            "emby", &TmdbPlaylistConfig::embyServers
         );
         // clang-format on
      };
   };

   struct TmdbSyncConfig
   {
      bool enabled{false};
      bool dryRun{false};
      std::string cron;
      std::string tmdbApiKey;
      std::vector<TmdbPlaylistConfig> playlists;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "enabled", &TmdbSyncConfig::enabled,
            "dry_run", &TmdbSyncConfig::dryRun,
            "cron", &TmdbSyncConfig::cron,
            "tmdb_api_key", &TmdbSyncConfig::tmdbApiKey,
            "playlists", &TmdbSyncConfig::playlists
         );
         // clang-format on
      };
   };

   struct ServerLibraryConfig
   {
      std::string server;
      std::string library;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "server", &ServerLibraryConfig::server,
            "library_name", &ServerLibraryConfig::library
         );
         // clang-format on
      };
   };

   struct FscIgnoreItem
   {
      std::string item;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "ignore", &FscIgnoreItem::item
         );
         // clang-format on
      };
   };

   struct FscEmptyFolderPathConfig
   {
      std::filesystem::path path;
      std::vector<ServerLibraryConfig> plex;
      std::vector<ServerLibraryConfig> emby;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "path", &FscEmptyFolderPathConfig::path,
            "plex", &FscEmptyFolderPathConfig::plex,
            "emby", &FscEmptyFolderPathConfig::emby
         );
         // clang-format on
      };
   };

   struct FscEmptyFolderDeleteConfig
   {
      std::vector<FscEmptyFolderPathConfig> paths;
      std::vector<FscIgnoreItem> ignoreFolders;
      std::vector<FscIgnoreItem> ignoreFileEmptyCheck;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "paths", &FscEmptyFolderDeleteConfig::paths,
            "ignore_folder_in_empty_check", &FscEmptyFolderDeleteConfig::ignoreFolders,
            "ignore_file_in_empty_check", &FscEmptyFolderDeleteConfig::ignoreFileEmptyCheck
         );
         // clang-format on
      };
   };

   struct FscDeleteFilePathConfig
   {
      std::filesystem::path path;
      int64_t deleteAgeHours{std::numeric_limits<int64_t>::max()};
      std::vector<ExtensionConfig> validExtensions;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "path", &FscDeleteFilePathConfig::path,
            "delete_age_hours", &FscDeleteFilePathConfig::deleteAgeHours,
            "valid_extensions_to_delete", &FscDeleteFilePathConfig::validExtensions
         );
         // clang-format on
      };
   };

   struct FscDeleteFilesByAgeConfig
   {
      std::vector<FscDeleteFilePathConfig> paths;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "paths", &FscDeleteFilesByAgeConfig::paths
         );
         // clang-format on
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
         // clang-format off
         static constexpr auto value = glz::object(
            "enabled", &FileSystemCleanupConfig::enabled,
            "dry_run", &FileSystemCleanupConfig::dryRun,
            "cron", &FileSystemCleanupConfig::cron,
            "empty_folder_delete", &FileSystemCleanupConfig::emptyFolderDeleteConfig,
            "delete_files_by_age", &FileSystemCleanupConfig::deleteFilesByAgeConfig
         );
         // clang-format on
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
         // clang-format off
         static constexpr auto value = glz::object(
            "plex", &DvrMaintainerLibrary::plex,
            "emby", &DvrMaintainerLibrary::emby,
            "path", &DvrMaintainerLibrary::path,
            "actions", &DvrMaintainerLibrary::actions,
            "extensions_to_delete", &DvrMaintainerLibrary::extensionsToDelete
         );
         // clang-format on
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
         // clang-format off
         static constexpr auto value = glz::object(
            "enabled", &DvrMaintainerConfig::enabled,
            "dry_run", &DvrMaintainerConfig::dryRun,
            "cron", &DvrMaintainerConfig::cron,
            "libraries", &DvrMaintainerConfig::libraries
         );
         // clang-format on
      };
   };

   struct DeleteWatchedUserConfig
   {
      std::string name;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "name", &DeleteWatchedUserConfig::name
         );
         // clang-format on
      };
   };

   struct DeleteWatchedServerConfig
   {
      std::string server;
      std::string library;
      std::filesystem::path mediaPath;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "server", &DeleteWatchedServerConfig::server,
            "library_name", &DeleteWatchedServerConfig::library,
            "media_path", &DeleteWatchedServerConfig::mediaPath
         );
         // clang-format on
      };
   };

   struct DeleteWatchedLibraryConfig
   {
      std::filesystem::path containerPath;
      std::vector<DeleteWatchedServerConfig> plex;
      std::vector<DeleteWatchedServerConfig> emby;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "container_path", &DeleteWatchedLibraryConfig::containerPath,
            "plex", &DeleteWatchedLibraryConfig::plex,
            "emby", &DeleteWatchedLibraryConfig::emby
         );
         // clang-format on
      };
   };

   struct DeleteWatchedTracearrUsers
   {
      std::string name;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "name", &DeleteWatchedTracearrUsers::name
         );
         // clang-format on
      };
   };

   struct DeleteWatchedConfig
   {
      bool enabled{false};
      bool dryRun{false};
      std::string cron;
      int32_t deleteTimeHours{24};
      std::vector<DeleteWatchedTracearrUsers> tracearrUsers;
      std::vector<DeleteWatchedLibraryConfig> libraries;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "enabled", &DeleteWatchedConfig::enabled,
            "dry_run", &DeleteWatchedConfig::dryRun,
            "cron", &DeleteWatchedConfig::cron,
            "delete_time_hours", &DeleteWatchedConfig::deleteTimeHours,
            "tracearr_users", &DeleteWatchedConfig::tracearrUsers,
            "libraries", &DeleteWatchedConfig::libraries
         );
         // clang-format on
      };
   };

   struct EmbyTidyLibraryConfig
   {
      std::string name;

      struct glaze
      {
         // clang-format off
         static constexpr auto value = glz::object(
            "library", &EmbyTidyLibraryConfig::name
         );
         // clang-format on
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
         // clang-format off
         static constexpr auto value = glz::object(
            "server", &EmbyTidyServerConfig::name,
            "clean_up_non_local_backdrops", &EmbyTidyServerConfig::cleanUpNonLocalBackdrops,
            "local_backdrop_container_start_path", &EmbyTidyServerConfig::localBackdropContainerStartPath,
            "libraries", &EmbyTidyServerConfig::libraries
         );
         // clang-format on
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
         // clang-format off
         static constexpr auto value = glz::object(
            "enabled", &EmbyTidyConfig::enabled,
            "dry_run", &EmbyTidyConfig::dryRun,
            "cron", &EmbyTidyConfig::cron,
            "servers", &EmbyTidyConfig::servers
         );
         // clang-format on
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
         // clang-format off
         static constexpr auto value = glz::object(
            "server_name", &TracearrServer::server_name,
            "url", &TracearrServer::url,
            "api_key", &TracearrServer::api_key
         );
         // clang-format on
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
      TmdbSyncConfig tmdbSync;

      struct glaze
      {
         // clang-format off
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
            "state_sync", &ConfigData::stateSync,
            "tmdb_sync", &ConfigData::tmdbSync
         );
         // clang-format on
      };
   };
} // namespace loomis
