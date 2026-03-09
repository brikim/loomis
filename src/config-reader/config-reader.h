#pragma once

#include "config-reader/config-reader-types.h"

#include <span>
#include <vector>

namespace loomis
{
   class ConfigReader
   {
   public:
      ConfigReader();
      ~ConfigReader() = default;

      [[nodiscard]] bool IsConfigValid() const;

      [[nodiscard]] const std::vector<ServerConfig>& GetPlexServers() const;
      [[nodiscard]] const std::vector<ServerConfig>& GetEmbyServers() const;
      [[nodiscard]] const AppriseLoggingConfig& GetAppriseLogging() const;
      [[nodiscard]] const GotifyLoggingConfig& GetGotifyLogging() const;
      [[nodiscard]] const PlaylistSyncConfig& GetPlaylistSyncConfig() const;
      [[nodiscard]] const WatchStateSyncConfig& GetWatchStateSyncConfig() const;
      [[nodiscard]] const FileSystemCleanupConfig& GetFilesystemCleanupConfig() const;
      [[nodiscard]] const DvrMaintainerConfig& GetDvrMaintainerConfig() const;
      [[nodiscard]] const DeleteWatchedConfig& GetDeleteWatchedConfig() const;
      [[nodiscard]] const EmbyTidyConfig& GetEmbyTidyConfig() const;

   private:
      void ReadConfigFile(const char* path);

      bool configValid_{false};
      ConfigData configData_;
   };
}