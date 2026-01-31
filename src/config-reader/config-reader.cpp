#include "config-reader.h"

#include <glaze/glaze.hpp>
#include <warp/log/log.h>
#include <warp/log/log-utils.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace loomis
{
   ConfigReader::ConfigReader()
   {
      //_putenv_s("CONFIG_PATH", "../config");
      if (const auto* configPath = std::getenv("CONFIG_PATH");
          configPath != nullptr)
      {
         ReadConfigFile(configPath);
      }
      else
      {
         warp::log::Error("CONFIG_PATH environment variable not found!");
      }
   }

   void ConfigReader::ReadConfigFile(const char* path)
   {
      std::filesystem::path pathFileName = std::filesystem::path(path) / "config.conf";
      if (!std::filesystem::exists(pathFileName))
      {
         warp::log::Error("Config file {} not found!", pathFileName.string());
         return;
      }

      auto ec = glz::read_file_json < glz::opts{.error_on_unknown_keys = false} > (
          configData_,
          pathFileName.string(),
          std::string{}
      );

      if (ec)
      {
         std::string pretty_error = glz::format_error(ec, "");
         warp::log::Warning("{} - Glaze Error: {} (File: {})",
                            __func__, pretty_error, pathFileName.string());
         return;
      }

      configValid_ = true;
   }

   bool ConfigReader::IsConfigValid() const
   {
      return configValid_;
   }

   const std::vector<ServerConfig>& ConfigReader::GetPlexServers() const
   {
      return configData_.plex.servers;
   }

   const std::vector<ServerConfig>& ConfigReader::GetEmbyServers() const
   {
      return configData_.emby.servers;
   }

   const AppriseLoggingConfig& ConfigReader::GetAppriseLogging() const
   {
      return configData_.apprise_logging;
   }

   const GotifyLoggingConfig& ConfigReader::GetGotifyLogging() const
   {
      return configData_.gotify_logging;
   }

   const PlaylistSyncConfig& ConfigReader::GetPlaylistSyncConfig() const
   {
      return configData_.playlist_sync;
   }

   const WatchStateSyncConfig& ConfigReader::GetWatchStateSyncConfig() const
   {
      return configData_.watch_state_sync;
   }

   const FolderCleanupConfig& ConfigReader::GetFolderCleanupConfig() const
   {
      return configData_.folder_cleanup;
   }

   const DvrMaintainerConfig& ConfigReader::GetDvrMaintainerConfig() const
   {
      return configData_.dvr_maintainer;
   }

   const DeleteWatchedConfig& ConfigReader::GetDeleteWatchedConfig() const
   {
      return configData_.delete_watched;
   }
}