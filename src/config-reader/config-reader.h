#pragma once

#include "config-reader/config-reader-types.h"
#include "types.h"

#include <json/json.hpp>
#include <span>
#include <vector>

namespace loomis
{
   class ConfigReader
   {
   public:
      ConfigReader();
      virtual ~ConfigReader() = default;

      [[nodiscard]] bool IsConfigValid() const;

      [[nodiscard]] const std::vector<ServerConfig>& GetPlexServers() const;
      [[nodiscard]] const std::vector<ServerConfig>& GetEmbyServers() const;
      [[nodiscard]] const AppriseLoggingConfig& GetAppriseLogging() const;
      [[nodiscard]] const PlaylistSyncConfig& GetPlaylistSyncConfig() const;
      [[nodiscard]] const WatchStateSyncConfig& GetWatchStateSyncConfig() const;

   private:
      void ReadConfigFile(const char* path);

      void ReadServerConfig(ApiType apiType, const nlohmann::json& jsonData, std::vector<ServerConfig>& servers);
      bool ReadServers(const nlohmann::json& jsonData);

      void ReadAppriseLogging(const nlohmann::json& jsonData);

      void ReadPlaylistSyncConfig(const nlohmann::json& jsonData);
      void ReadWatchStateSyncConfig(const nlohmann::json& jsonData);

      bool configValid_{false};
      ConfigData configData_;
   };
}