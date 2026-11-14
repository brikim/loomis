#pragma once

#include "config-reader/config-reader-types.h"

#include <string_view>
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
      [[nodiscard]] const TracearrServer& GetTracearrServer() const;
      [[nodiscard]] const AppriseLoggingConfig& GetAppriseLogging() const;
      [[nodiscard]] const GotifyLoggingConfig& GetGotifyLogging() const;
      [[nodiscard]] const PlaylistSyncConfig& GetPlaylistSyncConfig() const;
      [[nodiscard]] const FileSystemCleanupConfig& GetFilesystemCleanupConfig() const;
      [[nodiscard]] const DvrMaintainerConfig& GetDvrMaintainerConfig() const;
      [[nodiscard]] const DeleteWatchedConfig& GetDeleteWatchedConfig() const;
      [[nodiscard]] const EmbyTidyConfig& GetEmbyTidyConfig() const;
      [[nodiscard]] const StateSyncConfig& GetStateSyncConfig() const;
      [[nodiscard]] const TmdbSyncConfig& GetTmdbSyncConfig() const;

   private:
      void ReadConfigFile(const std::string_view path);

      bool configValid_{false};
      ConfigData configData_;
   };
} // namespace loomis
