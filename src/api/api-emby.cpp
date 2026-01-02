#include "api-emby.h"

#include "api/json-helper.h"
#include "logger/logger.h"
#include "logger/log-utils.h"
#include "types.h"

#include <json/json.hpp>

#include <format>
#include <numeric>
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

   std::string EmbyApi::BuildCommaSeparatedList(const std::vector<std::string>& list)
   {
      if (list.empty()) return "";

      return std::accumulate(std::next(list.begin()), list.end(), list[0],
         [](std::string a, const std::string& b) {
         return std::move(a) + "," + b;
      });
   }

   bool EmbyApi::GetValid()
   {
      auto res = client_.Get(BuildApiPath(API_SYSTEM_INFO), emptyHeaders_);
      return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
   }

   std::optional<std::string> EmbyApi::GetServerReportedName()
   {
      auto res = client_.Get(BuildApiPath(API_SYSTEM_INFO), emptyHeaders_);

      if (!IsHttpSuccess(__func__, res))
      {
         return std::nullopt;
      }

      auto data = JsonSafeParse(res.value().body);
      if (!data.has_value())
      {
         return std::nullopt;
      }

      auto serverName = JsonSafeGet<std::string>(data.value(), SERVER_NAME);
      if (serverName.has_value())
      {
         return std::move(serverName).value();
      }

      return std::nullopt;
   }

   std::optional<std::string> EmbyApi::GetLibraryId(std::string_view libraryName)
   {
      auto res = client_.Get(BuildApiPath(API_MEDIA_FOLDERS), emptyHeaders_);

      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      auto data = JsonSafeParse(res.value().body);
      if (!data || !data->is_array()) return std::nullopt;

      for (const auto& library : *data)
      {
         // Compare directly against the JSON internal string
         // This avoids creating a new std::string for every iteration
         if (library.contains(NAME) && library[NAME].get_ref<const std::string&>() == libraryName)
         {
            auto libId = JsonSafeGet<std::string>(library, ID);
            if (libId) return std::move(libId).value();
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

      if (!extraSearchArgs.empty()) AddApiParam(apiUrl, extraSearchArgs);

      auto res{client_.Get(apiUrl, emptyHeaders_)};
      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      auto data = JsonSafeParse(res.value().body);
      if (!data || !data->contains(ITEMS) || !(*data)[ITEMS].is_array()) return std::nullopt;

      std::string_view jsonKey;
      switch (type)
      {
         case EmbySearchType::id:   jsonKey = ID;   break; // Search "Ids", but check "Id"
         case EmbySearchType::path: jsonKey = PATH; break; // Search "Path", check "Path"
         case EmbySearchType::name: jsonKey = NAME; break; // Search "SearchTerm", check "Name"
      }

      for (const auto& item : (*data)[ITEMS])
      {
         bool isMatch = false;
         if (item.contains(jsonKey) && item[jsonKey].is_string())
         {
            isMatch = (item[jsonKey].get_ref<const std::string&>() == name);
         }

         if (isMatch)
         {
            EmbyItem returnItem;

            // Helper to fill the struct using your existing JsonSafeGet logic
            // We use std::move because these are temporary values being put into our struct
            if (auto val = JsonSafeGet<std::string>(item, ID))   returnItem.id = std::move(*val);
            if (auto val = JsonSafeGet<std::string>(item, TYPE)) returnItem.type = std::move(*val);
            if (auto val = JsonSafeGet<std::string>(item, NAME)) returnItem.name = std::move(*val);
            if (auto val = JsonSafeGet<std::string>(item, PATH)) returnItem.path = std::move(*val);

            if (auto val = JsonSafeGet<std::string>(item, SERIES_NAME)) returnItem.series.name = std::move(*val);

            returnItem.series.seasonNum = JsonSafeGet<uint32_t>(item, PARENT_INDEX_NUMBER).value_or(0u);
            returnItem.series.episodeNum = JsonSafeGet<uint32_t>(item, INDEX_NUMBER).value_or(0u);
            returnItem.runTimeTicks = JsonSafeGet<uint64_t>(item, RUN_TIME_TICKS).value_or(0u);

            return returnItem;
         }
      }

      LogWarning(std::format("GetItem returned no valid results {}", utils::GetTag("search", name)));
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

      // 1. Guard Clause: Return false immediately if the HTTP call fails
      if (!IsHttpSuccess(__func__, res)) return false;

      // 2. Parse JSON and verify it's an array
      auto data = JsonSafeParse(res.value().body);
      if (!data || !data->is_array()) return false;

      // 3. Optimized Loop: No heap allocations
      for (const auto& user : *data)
      {
         // Verify key exists and is a string
         if (user.contains(NAME)
             && user[NAME].is_string()
             && user[NAME].get_ref<const std::string&>() == name)
         {
            return true;
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
      auto item = GetItem(EmbySearchType::name, name, {{"IncludeItemTypes", "Playlist"}});
      if (!item.has_value()) return std::nullopt;

      auto apiUrl = BuildApiPath(std::format("{}/{}/Items", API_PLAYLISTS, item->id));
      auto res = client_.Get(apiUrl, emptyHeaders_);

      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      auto jsonData = JsonSafeParse(res.value().body);
      if (!jsonData || !jsonData->contains(ITEMS) || !(*jsonData)[ITEMS].is_array())
      {
         return std::nullopt;
      }

      EmbyPlaylist returnPlaylist;
      returnPlaylist.name = std::move(item->name);
      returnPlaylist.id = std::move(item->id);

      const auto& itemsArray = (*jsonData)[ITEMS];
      returnPlaylist.items.reserve(itemsArray.size()); // Optimization: Prevent vector reallocations

      for (const auto& jsonItem : itemsArray)
      {
         auto& pItem = returnPlaylist.items.emplace_back();

         // Use std::move to transfer strings from the JSON temporary directly into the struct
         if (auto val = JsonSafeGet<std::string>(jsonItem, ID))
            pItem.id = std::move(*val);

         if (auto val = JsonSafeGet<std::string>(jsonItem, NAME))
            pItem.name = std::move(*val);

         if (auto val = JsonSafeGet<std::string>(jsonItem, PLAYLIST_ITEM_ID))
            pItem.playlistId = std::move(*val);
      }

      return returnPlaylist;
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
      IsHttpSuccess(__func__, res);
   }

   bool EmbyApi::AddPlaylistItems(std::string_view playlistId, const std::vector<std::string>& addIds)
   {
      auto apiUrl{BuildApiPath(std::format("{}/{}/Items", API_PLAYLISTS, playlistId))};
      AddApiParam(apiUrl, {
         {IDS, BuildCommaSeparatedList(addIds)}
      });
      auto res{client_.Post(apiUrl, jsonHeaders_)};
      return IsHttpSuccess(__func__, res);
   }

   bool EmbyApi::RemovePlaylistItems(std::string_view playlistId, const std::vector<std::string>& removeIds)
   {
      auto apiUrl{BuildApiPath(std::format("{}/{}/Items/Delete", API_PLAYLISTS, playlistId))};
      AddApiParam(apiUrl, {
         {ENTRY_IDS, BuildCommaSeparatedList(removeIds)}
      });
      auto res{client_.Post(apiUrl, jsonHeaders_)};
      return IsHttpSuccess(__func__, res);
   }

   bool EmbyApi::MovePlaylistItem(std::string_view playlistId, std::string_view itemId, uint32_t index)
   {
      auto apiUrl{BuildApiPath(std::format("{}/{}/Items/{}/Move/{}", API_PLAYLISTS, playlistId, itemId, index))};
      auto res{client_.Post(apiUrl, jsonHeaders_)};
      return IsHttpSuccess(__func__, res);
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
      IsHttpSuccess(__func__, res);
   }
}