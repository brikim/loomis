#include "api-emby.h"

#include "api/json-helper.h"
#include "logger/logger.h"
#include "logger/log-utils.h"
#include "types.h"

#include <format>
#include <ranges>

namespace loomis
{
   namespace
   {
      const std::string API_BASE{"/emby"};
      const std::string API_SYSTEM_INFO{"/System/Info"};
      const std::string API_MEDIA_FOLDERS{"/Library/SelectableMediaFolders"};
      const std::string API_ITEMS{"/Items"};
      const std::string API_PLAYLISTS{"/Playlists"};
      const std::string API_USERS{"/Users"};

      constexpr std::string_view SERVER_NAME{"ServerName"};
      constexpr std::string_view NAME{"Name"};
      constexpr std::string_view ID{"Id"};
      constexpr std::string_view IDS{"Ids"};
      constexpr std::string_view ITEMS{"Items"};
      constexpr std::string_view TYPE{"Type"};
      constexpr std::string_view PATH{"Path"};

      constexpr std::string_view TOTAL_RECORD_COUNT{"TotalRecordCount"};
      constexpr std::string_view MEDIA_TYPE{"MediaType"};
      constexpr std::string_view MOVIES{"Movies"};
      constexpr std::string_view SEARCH_TERM{"SearchTerm"};
      constexpr std::string_view SERIES_NAME{"SeriesName"};
      constexpr std::string_view PARENT_INDEX_NUMBER{"ParentIndexNumber"};
      constexpr std::string_view INDEX_NUMBER{"IndexNumber"};
      constexpr std::string_view RUN_TIME_TICKS{"RunTimeTicks"};
      constexpr std::string_view PLAYLIST_ITEM_ID{"PlaylistItemId"};
      constexpr std::string_view ENTRY_IDS{"EntryIds"};
   }

   EmbyApi::EmbyApi(const ServerConfig& serverConfig)
      : ApiBase(serverConfig.name, serverConfig.main, "EmbyApi", utils::ANSI_CODE_EMBY)
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
         if (auto data = JsonSafeParse(res.value().body);
             data.has_value())
         {
            auto serverName = JsonSafeGet<std::string>(data.value(), SERVER_NAME);
            if (serverName.has_value())
            {
               return serverName.value();
            }
         }
      }
      return std::nullopt;
   }

   std::optional<std::string> EmbyApi::GetLibraryId(std::string_view libraryName)
   {
      auto res = client_.Get(BuildApiPath(API_MEDIA_FOLDERS), emptyHeaders_);
      if (IsHttpSuccess("GetLibraryId", res))
      {
         if (auto data = JsonSafeParse(res.value().body);
             data.has_value() && data.value().is_array())
         {
            for (const auto& library : data.value())
            {
               auto libName{JsonSafeGet<std::string>(library, NAME)};
               if (libName.has_value() && libName.value() == libraryName)
               {
                  if (auto libId = JsonSafeGet<std::string>(library, ID);
                      libId.has_value())
                  {
                     return libId.value();
                  }
               }
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
         {GetSearchTypeStr(type), GetPercentEncoded(name)},
         {"Fields", "Path"}
      });
      if (!extraSearchArgs.empty())
      {
         AddApiParam(apiUrl, extraSearchArgs);
      }

      auto res{client_.Get(apiUrl, emptyHeaders_)};
      if (IsHttpSuccess("GetItem", res))
      {
         if (auto data = JsonSafeParse(res.value().body);
             data.has_value() && data.value().contains(ITEMS) && data.value()[ITEMS].is_array())
         {
            for (const auto& item : data.value()[ITEMS])
            {
               auto itemId = JsonSafeGet<std::string>(item, ID);
               auto itemPath = JsonSafeGet<std::string>(item, PATH);
               auto itemName = JsonSafeGet<std::string>(item, NAME);
               if ((type == EmbySearchType::id && itemId.has_value() && itemId.value() == name)
                   || (type == EmbySearchType::path && itemPath.has_value() && itemPath.value() == name)
                   || (type == EmbySearchType::name && itemName.has_value() && itemName == name))
               {
                  auto itemType = JsonSafeGet<std::string>(item, TYPE);
                  auto itemSeriesName = JsonSafeGet<std::string>(item, SERIES_NAME);
                  auto itemSeasonNum = JsonSafeGet<uint32_t>(item, PARENT_INDEX_NUMBER);
                  auto itemEpisodeNum = JsonSafeGet<uint32_t>(item, INDEX_NUMBER);
                  auto itemRuntimeTicks = JsonSafeGet<uint64_t>(item, RUN_TIME_TICKS);

                  EmbyItem returnItem;
                  if (itemId.has_value()) returnItem.id = itemId.value();
                  if (itemType.has_value()) returnItem.type = itemType.value();
                  if (itemName.has_value()) returnItem.name = itemName.value();
                  if (itemPath.has_value()) returnItem.path = itemPath.value();
                  if (itemSeriesName.has_value()) returnItem.series.name = itemSeriesName.value();
                  if (itemSeasonNum.has_value()) returnItem.series.seasonNum = itemSeasonNum.value();
                  if (itemEpisodeNum.has_value()) returnItem.series.episodeNum = itemEpisodeNum.value();
                  if (itemRuntimeTicks.has_value()) returnItem.runTimeTicks = itemRuntimeTicks.value();

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

   bool EmbyApi::GetUserExists(std::string_view name)
   {
      auto res = client_.Get(BuildApiPath(API_USERS), emptyHeaders_);
      if (IsHttpSuccess("GetUserExists", res))
      {
         auto data = JsonSafeParse(res.value().body);
         if (data.has_value() && data.value().is_array())
         {
            for (const auto& user : data.value())
            {
               auto userName = JsonSafeGet<std::string>(user, NAME);
               if (userName.has_value())
               {
                  if (*userName == name)
                  {
                     return true;
                  }
               }
            }
         }
      }
      return false;
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
            if (auto jsonData = JsonSafeParse(res.value().body);
                jsonData.has_value())
            {
               const auto& data = jsonData.value();
               if (data.contains(ITEMS) && data[ITEMS].is_array())
               {
                  EmbyPlaylist returnPlaylist;
                  returnPlaylist.name = item.value().name;
                  returnPlaylist.id = item.value().id;
                  for (const auto& item : data[ITEMS])
                  {
                     auto& playlistItem{returnPlaylist.items.emplace_back()};

                     auto itemId = JsonSafeGet<std::string>(item, ID);
                     auto itemName = JsonSafeGet<std::string>(item, NAME);
                     auto itemPlaylistId = JsonSafeGet<std::string>(item, PLAYLIST_ITEM_ID);
                     if (itemId.has_value()) playlistItem.id = itemId.value();
                     if (itemName.has_value()) playlistItem.name = itemName.value();
                     if (itemPlaylistId.has_value()) playlistItem.playlistId = itemPlaylistId.value();
                  }

                  return returnPlaylist;
               }
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
}