#include "config-reader.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

#include <cstdlib>
#include <format>
#include <fstream>

namespace loomis
{
   constexpr const std::string_view PLEX{"plex"};
   constexpr const std::string_view EMBY{"emby"};
   constexpr const std::string_view SERVERS{"servers"};
   constexpr const std::string_view SERVER_NAME{"server_name"};
   constexpr const std::string_view URL{"url"};
   constexpr const std::string_view API_KEY{"api_key"};
   constexpr const std::string_view TRACKER_URL{"tracker_url"};
   constexpr const std::string_view TRACKER_API_KEY{"tracker_api_key"};
   constexpr const std::string_view MEDIA_PATH{"media_path"};
   constexpr const std::string_view APPRISE_LOGGING{"apprise_logging"};
   constexpr const std::string_view ENABLED("enabled");
   constexpr const std::string_view KEY("key");
   constexpr const std::string_view MESSAGE_TITLE("message_title");
   constexpr const std::string_view PLAYLIST_SYNC("playlist_sync");
   constexpr const std::string_view CRON("cron");
   constexpr const std::string_view PLEX_COLLECTION_SYNC("plex_collection_sync");
   constexpr const std::string_view SERVER("server");
   constexpr const std::string_view LIBRARY("library");
   constexpr const std::string_view COLLECTION_NAME("collection_name");
   constexpr const std::string_view TARGET_EMBY_SERVERS("target_emby_servers");
   constexpr const std::string_view TIME_FOR_EMBY_TO_UPDATE("time_for_emby_to_update_seconds");
   constexpr const std::string_view TIME_BETWEEN_SYNCS("time_between_syncs_seconds");

   constexpr const std::string_view DEFAULT_CRON("0 */2");

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
         Logger::Instance().Error(std::format("Config file {} not found!", pathFileName));
         return;
      }

      try
      {
         auto jsonData = nlohmann::json::parse(f);

         if (ReadServers(jsonData) == false)
         {
            Logger::Instance().Error("No Servers loaded exiting");
            return;
         }

         // The configuration file is valid
         configValid_ = true;

         ReadAppriseLogging(jsonData);
         ReadPlaylistSyncConfig(jsonData);
      }
      catch (const std::exception& e)
      {
         Logger::Instance().Error(std::format("{} parsing has an error {}", pathFileName, e.what()));
         return;
      }
   }

   void ConfigReader::ReadServerConfig(ApiType apiType, const nlohmann::json& jsonData, std::vector<ServerConfig>& servers)
   {
      if (jsonData.contains(SERVERS) == false)
      {
         return;
      }

      for (auto& serverConfigJson : jsonData[SERVERS])
      {
         if (serverConfigJson.contains(SERVER_NAME) == false
             || serverConfigJson.contains(URL) == false
             || serverConfigJson.contains(API_KEY) == false
             || serverConfigJson.contains(MEDIA_PATH) == false)
         {
            Logger::Instance().Error(std::format("{} server config invalid {} {} {} {}",
                                                 utils::GetFormattedApiName(apiType),
                                                 utils::GetTag(SERVER_NAME, serverConfigJson.contains(SERVER_NAME) ? serverConfigJson[SERVER_NAME].get<std::string>() : "ERROR"),
                                                 utils::GetTag(URL, serverConfigJson.contains(URL) ? serverConfigJson[URL].get<std::string>() : "ERROR"),
                                                 utils::GetTag(API_KEY, serverConfigJson.contains(API_KEY) ? serverConfigJson[API_KEY].get<std::string>() : "ERROR"),
                                                 utils::GetTag(MEDIA_PATH, serverConfigJson.contains(MEDIA_PATH) ? serverConfigJson[MEDIA_PATH].get<std::string>() : "ERROR")));
            break;
         }

         auto& serverConfig{servers.emplace_back()};
         serverConfig.name = serverConfigJson[SERVER_NAME].get<std::string>();
         serverConfig.main.valid = true;
         serverConfig.main.url = serverConfigJson[URL].get<std::string>();
         serverConfig.main.apiKey = serverConfigJson[API_KEY].get<std::string>();
         serverConfig.mediaPath = serverConfigJson[MEDIA_PATH].get<std::string>();

         if (serverConfigJson.contains(TRACKER_URL) && serverConfigJson.contains(TRACKER_API_KEY))
         {
            serverConfig.tracker.valid = true;
            serverConfig.tracker.url = serverConfigJson[TRACKER_URL].get<std::string>();
            serverConfig.tracker.apiKey = serverConfigJson[TRACKER_API_KEY].get<std::string>();
         }
         else
         {
            Logger::Instance().Warning(std::format("Tracker {}/{} not set for {}. Some services may not be available", TRACKER_URL, TRACKER_API_KEY, utils::GetFormattedApiName(apiType)));
         }
      }
   }

   bool ConfigReader::ReadServers(const nlohmann::json& jsonData)
   {
      if (jsonData.contains(PLEX))
      {
         ReadServerConfig(ApiType::PLEX, jsonData[PLEX], configData_.plexServers);
      }

      if (jsonData.contains(EMBY))
      {
         ReadServerConfig(ApiType::EMBY, jsonData[EMBY], configData_.embyServers);
      }

      return (configData_.plexServers.size() + configData_.embyServers.size()) > 0;
   }

   void ConfigReader::ReadAppriseLogging(const nlohmann::json& jsonData)
   {
      if (jsonData.contains(APPRISE_LOGGING) == false
          || jsonData[APPRISE_LOGGING].contains(ENABLED) == false
          || jsonData[APPRISE_LOGGING][ENABLED].get<std::string>() != "True"
          || jsonData[APPRISE_LOGGING].contains(URL) == false
          || jsonData[APPRISE_LOGGING].contains(KEY) == false
          || jsonData[APPRISE_LOGGING].contains(MESSAGE_TITLE) == false)
      {
         return;
      }

      configData_.appriseLogging.enabled = true;
      configData_.appriseLogging.url = jsonData[APPRISE_LOGGING][URL].get<std::string>();
      configData_.appriseLogging.key = jsonData[APPRISE_LOGGING][KEY].get<std::string>();
      configData_.appriseLogging.title = jsonData[APPRISE_LOGGING][MESSAGE_TITLE].get<std::string>();
   }

   void ConfigReader::ReadPlaylistSyncConfig(const nlohmann::json& jsonData)
   {
      if (jsonData.contains(PLAYLIST_SYNC) == false
          || jsonData[PLAYLIST_SYNC].contains(ENABLED) == false
          || jsonData[PLAYLIST_SYNC][ENABLED].get<std::string>() != "True")
      {
         return;
      }

      const auto& playlistSyncJson{jsonData[PLAYLIST_SYNC]};

      if (playlistSyncJson.contains(CRON))
      {
         configData_.playlistSync.cron = playlistSyncJson[CRON].get<std::string>();
      }
      else
      {
         configData_.playlistSync.cron = DEFAULT_CRON;
         Logger::Instance().Warning(std::format("Playlist Sync Config missing cron. Using default {}", DEFAULT_CRON));
      }

      if (playlistSyncJson.contains(TIME_FOR_EMBY_TO_UPDATE))
      {
         configData_.playlistSync.timeForEmbyUpdateSec = playlistSyncJson[TIME_FOR_EMBY_TO_UPDATE].get<uint32_t>();
      }

      if (playlistSyncJson.contains(TIME_BETWEEN_SYNCS))
      {
         configData_.playlistSync.timeBetweenSyncSec = playlistSyncJson[TIME_BETWEEN_SYNCS].get<uint32_t>();
      }

      if (playlistSyncJson.contains(PLEX_COLLECTION_SYNC))
      {
         auto index{0};
         for (const auto& plexCollection : playlistSyncJson[PLEX_COLLECTION_SYNC])
         {
            if (plexCollection.contains(SERVER) == false
                || plexCollection.contains(LIBRARY) == false
                || plexCollection.contains(COLLECTION_NAME) == false
                || plexCollection.contains(TARGET_EMBY_SERVERS) == false)
            {
               Logger::Instance().Warning(std::format("Playlist Sync Config error reading in plex collection {}", utils::GetTag("index", std::to_string(index))));
               continue;
            }

            PlaylistPlexCollection playlistPlexCollection;
            playlistPlexCollection.server = plexCollection[SERVER].get<std::string>();
            playlistPlexCollection.library = plexCollection[LIBRARY].get<std::string>();
            playlistPlexCollection.collectionName = plexCollection[COLLECTION_NAME].get<std::string>();

            for (const auto& targetEmbyServer : plexCollection[TARGET_EMBY_SERVERS])
            {
               if (targetEmbyServer.contains(SERVER))
               {
                  playlistPlexCollection.embyServers.emplace_back(targetEmbyServer[SERVER].get<std::string>());
               }
               else
               {
                  Logger::Instance().Warning(std::format("Playlist Sync Config - Error reading in plex collection! Each {} requires a {} {} ", TARGET_EMBY_SERVERS, SERVER, utils::GetTag("index", std::to_string(index))));
               }
            }

            if (playlistPlexCollection.embyServers.size() > 0)
            {
               configData_.playlistSync.plexCollections.emplace_back(playlistPlexCollection);
            }

            ++index;
         }
      }

      if (configData_.playlistSync.plexCollections.size() > 0)
      {
         configData_.playlistSync.enabled = true;
      }
      else
      {
         Logger::Instance().Warning("Playlist Sync Config - Enabled but no valid collections to sync");
      }
   }

   bool ConfigReader::IsConfigValid() const
   {
      return configValid_;
   }

   const std::vector<ServerConfig>& ConfigReader::GetPlexServers() const
   {
      return configData_.plexServers;
   }

   const std::vector<ServerConfig>& ConfigReader::GetEmbyServers() const
   {
      return configData_.embyServers;
   }

   const AppriseLoggingConfig& ConfigReader::GetAppriseLogging() const
   {
      return configData_.appriseLogging;
   }

   const PlaylistSyncConfig& ConfigReader::GetPlaylistSyncConfig() const
   {
      return configData_.playlistSync;
   }
}