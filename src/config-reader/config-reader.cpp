#include "config-reader.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

#include "glaze/glaze.hpp"

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
         Logger::Instance().Error("CONFIG_PATH environment variable not found!");
      }
   }

   void ConfigReader::ReadConfigFile(const char* path)
   {
      std::filesystem::path pathFileName = std::filesystem::path(path) / "config.conf";
      std::ifstream file(pathFileName, std::ios::in | std::ios::binary);

      if (!file.is_open())
      {
         Logger::Instance().Error("Config file {} not found!", pathFileName.string());
         return;
      }

      if (auto ec = glz::read_file_json < glz::opts{.error_on_unknown_keys = false} > (
         configData_,
         pathFileName.string(),
         std::string{}))
      {
         Logger::Instance().Warning("{} - Glaze Error: {} (File: {})",
                                    __func__, static_cast<int>(ec.ec), pathFileName.string());
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
}