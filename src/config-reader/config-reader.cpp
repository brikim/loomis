#include "config-reader.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

#include "glaze/glaze.hpp"

#include <cstdlib>
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
      std::string pathFileName{path};
      pathFileName.append("/config.conf");

      std::ifstream f(pathFileName);
      if (f.is_open() == false)
      {
         Logger::Instance().Error("Config file {} not found!", pathFileName);
         return;
      }

      std::ifstream file(pathFileName, std::ios::binary);
      if (!file)
      {
         Logger::Instance().Error("Config file {} not found!", pathFileName);
         return;
      }

      // Read the entire file content into a string (common approach for small/medium files)
      std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
      if (auto ec = glz::read < glz::opts{.error_on_unknown_keys = false} > (configData_, content))
      {
         Logger::Instance().Warning("{} - JSON Parse Error: {}",
                                    __func__, glz::format_error(ec, content));
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
}