#include "api-emby.h"

#include "logger/logger.h"
#include "logger/log-utils.h"
#include "types.h"

#include <format>
#include <ranges>

namespace loomis
{
   static const std::string API_BASE{"/emby"};
   static const std::string API_SYSTEM_INFO{"/System/Info"};
   static const std::string API_MEDIA_FOLDERS{"/Library/SelectableMediaFolders"};
   static const std::string API_ITEMS{"/Items"};
   static const std::string API_PLAYLISTS{"/Playlists"};

   static constexpr std::string_view SERVER_NAME{"ServerName"};
   static constexpr std::string_view NAME{"Name"};
   static constexpr std::string_view ID{"Id"};
   static constexpr std::string_view IDS{"Ids"};
   static constexpr std::string_view ITEMS{"Items"};
   static constexpr std::string_view TYPE{"Type"};
   static constexpr std::string_view PATH{"Path"};

   static constexpr std::string_view TOTAL_RECORD_COUNT{"TotalRecordCount"};
   static constexpr std::string_view MEDIA_TYPE{"MediaType"};
   static constexpr std::string_view MOVIES{"Movies"};
   static constexpr std::string_view SEARCH_TERM{"SearchTerm"};
   static constexpr std::string_view SERIES_NAME{"SeriesName"};
   static constexpr std::string_view PARENT_INDEX_NUMBER{"ParentIndexNumber"};
   static constexpr std::string_view INDEX_NUMBER{"IndexNumber"};
   static constexpr std::string_view RUN_TIME_TICKS{"RunTimeTicks"};
   static constexpr std::string_view PLAYLIST_ITEM_ID{"PlaylistItemId"};
   static constexpr std::string_view ENTRY_IDS{"EntryIds"};

   EmbyApi::EmbyApi(const ServerConfig& serverConfig)
      : ApiBase(serverConfig, "EmbyApi", utils::ANSI_CODE_EMBY)
      , client_(GetUrl())
   {
      constexpr time_t timeoutSec{5};
      client_.set_connection_timeout(timeoutSec);
   }

   std::string EmbyApi::BuildApiPath(std::string_view path) const
   {
      return std::format("{}{}?api_key={}", API_BASE, path, GetApiKey());
   }

   void EmbyApi::AddApiParam(std::string& url, const std::list<std::pair<std::string_view, std::string_view>>& params) const
   {
      std::ranges::for_each(params, [&url](const auto& param) {
         url.append(std::format("&{}={}", param.first, param.second));
      });
   }

   std::string EmbyApi::BuildCommaSeparatedList(const std::vector<std::string>& list)
   {
      std::string returnList;
      for (const auto& item : list)
      {
         if (!returnList.empty())
         {
            returnList.append(",");
         }
         returnList.append(item);
      }
      return returnList;
   }

   bool EmbyApi::GetValid()
   {
      auto res = client_.Get(BuildApiPath(API_SYSTEM_INFO), emptyHeaders_);
      return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
   }

   std::optional<std::string> EmbyApi::GetServerReportedName()
   {
      auto res = client_.Get(BuildApiPath(API_SYSTEM_INFO), emptyHeaders_);
      if (IsHttpSuccess("GetServerReportedName", res))
      {
         auto data = nlohmann::json::parse(res.value().body);
         if (data.contains(SERVER_NAME))
         {
            return data.at(SERVER_NAME).get<std::string>();
         }
      }
      return std::nullopt;
   }

   std::optional<std::string> EmbyApi::GetLibraryId(std::string_view libraryName)
   {
      auto res = client_.Get(BuildApiPath(API_MEDIA_FOLDERS), emptyHeaders_);
      if (IsHttpSuccess("GetLibraryId", res))
      {
         auto data = nlohmann::json::parse(res.value().body);
         for (const auto& library : data)
         {
            if (library.contains(NAME) && library.at(NAME).get<std::string>() == libraryName && library.contains(ID))
            {
               return library.at(ID).get<std::string>();
            }
         }
      }
      return std::nullopt;
   }

   std::string_view EmbyApi::GetSearchTypeStr(EmbySearchType type)
   {
      switch (type)
      {
         case EmbySearchType::id:
            return IDS;
         case EmbySearchType::path:
            return PATH;
         default:
            return SEARCH_TERM;
      }
   }

   std::optional<EmbyItem> EmbyApi::GetItem(EmbySearchType type, std::string_view name, std::list<std::pair<std::string_view, std::string_view>> extraSearchArgs)
   {
      auto apiUrl{BuildApiPath(API_ITEMS)};
      AddApiParam(apiUrl, {
         {"Recursive", "true"},
         {GetSearchTypeStr(type), name},
         {"Fields", "Path"}
      });
      if (!extraSearchArgs.empty())
      {
         AddApiParam(apiUrl, extraSearchArgs);
      }

      auto res{client_.Get(apiUrl, emptyHeaders_)};
      if (IsHttpSuccess("GetItem", res))
      {
         auto data{nlohmann::json::parse(res.value().body)};
         if (data.contains(ITEMS))
         {
            for (const auto& item : data[ITEMS])
            {
               if ((type == EmbySearchType::id && item.contains(ID) && item[ID].get<std::string>() == name)
                   || (type == EmbySearchType::path && item.contains(PATH) && item[PATH].get<std::string>() == name)
                   || (type == EmbySearchType::name && item.contains(NAME) && item[NAME].get<std::string>() == name))
               {
                  EmbyItem returnItem;
                  if (item.contains(ID)) returnItem.id = item[ID].get<std::string>();
                  if (item.contains(TYPE)) returnItem.type = item[TYPE].get<std::string>();
                  if (item.contains(NAME)) returnItem.name = item[NAME].get<std::string>();
                  if (item.contains(PATH)) returnItem.path = item[PATH].get<std::string>();
                  if (item.contains(SERIES_NAME)) returnItem.series.name = item[SERIES_NAME].get<std::string>();
                  if (item.contains(PARENT_INDEX_NUMBER)) returnItem.series.seasonNum = item[PARENT_INDEX_NUMBER].get<uint32_t>();
                  if (item.contains(INDEX_NUMBER)) returnItem.series.episodeNum = item[INDEX_NUMBER].get<uint32_t>();
                  if (item.contains(RUN_TIME_TICKS)) returnItem.runTimeTicks = item[RUN_TIME_TICKS].get<uint64_t>();

                  return returnItem;
               }
            }
         }

         LogWarning(std::format("GetItem returned no valid results {}",
                                utils::GetTag("search", name)));
      }

      return std::nullopt;
   }

   std::optional<std::string> EmbyApi::GetItemIdFromPath(std::string_view path)
   {
      if (auto item{GetItem(EmbySearchType::path, path)};
          item.has_value())
      {
         return item.value().id;
      }
      return std::nullopt;
   }

   bool EmbyApi::GetPlaylistExists(std::string_view name)
   {
      return GetItem(EmbySearchType::name, name, {{"IncludeItemTypes", "Playlist"}}).has_value();
   }

   std::optional<EmbyPlaylist> EmbyApi::GetPlaylist(std::string_view name)
   {
      auto item{GetItem(EmbySearchType::name, name, {{"IncludeItemTypes", "Playlist"}})};
      if (item.has_value())
      {
         auto apiUrl{BuildApiPath(std::format("{}/{}/Items", API_PLAYLISTS, item.value().id))};
         auto res{client_.Get(apiUrl, emptyHeaders_)};
         if (IsHttpSuccess("GetItem", res))
         {
            auto data{nlohmann::json::parse(res.value().body)};
            if (data.contains(ITEMS))
            {
               EmbyPlaylist returnPlaylist;
               returnPlaylist.name = item.value().name;
               returnPlaylist.id = item.value().id;
               for (const auto& item : data[ITEMS])
               {
                  auto& playlistItem{returnPlaylist.items.emplace_back()};
                  if (item.contains(ID)) playlistItem.id = item[ID].get<std::string>();
                  if (item.contains(NAME)) playlistItem.name = item[NAME].get<std::string>();
                  if (item.contains(PLAYLIST_ITEM_ID)) playlistItem.playlistId = item[PLAYLIST_ITEM_ID].get<std::string>();
               }

               return returnPlaylist;
            }
         }
      }

      return std::nullopt;
   }

   void EmbyApi::CreatePlaylist(std::string_view name, const std::vector<std::string>& itemIds)
   {
      auto apiUrl = BuildApiPath(API_PLAYLISTS);
      AddApiParam(apiUrl, {
         {NAME, name},
         {IDS, BuildCommaSeparatedList(itemIds)},
         {MEDIA_TYPE, MOVIES}
      });

      auto res{client_.Post(apiUrl, jsonHeaders_)};
      IsHttpSuccess("CreatePlaylist", res);
   }

   bool EmbyApi::AddPlaylistItems(std::string_view playlistId, const std::vector<std::string>& addIds)
   {
      auto apiUrl{BuildApiPath(std::format("{}/{}/Items", API_PLAYLISTS, playlistId))};
      AddApiParam(apiUrl, {
         {IDS, BuildCommaSeparatedList(addIds)}
      });
      auto res{client_.Post(apiUrl, jsonHeaders_)};
      return IsHttpSuccess("AddPlaylistItems", res);
   }

   bool EmbyApi::RemovePlaylistItems(std::string_view playlistId, const std::vector<std::string>& removeIds)
   {
      auto apiUrl{BuildApiPath(std::format("{}/{}/Items/Delete", API_PLAYLISTS, playlistId))};
      AddApiParam(apiUrl, {
         {ENTRY_IDS, BuildCommaSeparatedList(removeIds)}
      });
      auto res{client_.Post(apiUrl, jsonHeaders_)};
      return IsHttpSuccess("RemovePlaylistItems", res);
   }

   bool EmbyApi::MovePlaylistItem(std::string_view playlistId, std::string_view itemId, uint32_t index)
   {
      auto apiUrl{BuildApiPath(std::format("{}/{}/Items/{}/Move/{}", API_PLAYLISTS, playlistId, itemId, index))};
      auto res{client_.Post(apiUrl, jsonHeaders_)};
      return IsHttpSuccess("RemovePlaylistItems", res);
   }

   void EmbyApi::SetLibraryScan(std::string_view libraryId)
   {
      httplib::Headers headers = {
         {"accept", "*/*"}
      };

      auto apiUrl = BuildApiPath(std::format("/Items/{}/Refresh", libraryId));
      AddApiParam(apiUrl, {
         {"Recursive", "true"},
         {"ImageRefreshMode", "Default"},
         //{"MetadataRefreshMode", ""}, // optional paramater
         {"ReplaceAllImages", "false"},
         {"ReplaceAllMetadata", "false"}
      });

      auto res = client_.Post(apiUrl, headers);
      IsHttpSuccess("SetLibraryScan", res);
   }

   bool EmbyApi::IsHttpSuccess(std::string_view name, const httplib::Result& result)
   {
      if (result.error() != httplib::Error::Success || result.value().status >= VALID_HTTP_RESPONSE_MAX)
      {
         LogWarning(std::format("{} - HTTP error {}",
                                name,
                                utils::GetTag("error",
                                              result.value().status >= VALID_HTTP_RESPONSE_MAX ? std::format("{} - {}", result.value().reason, result.value().body) : httplib::to_string(result.error()))));
         return false;
      }
      return true;
   }
}