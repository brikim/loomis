#pragma once

#include "api/api-base.h"
#include "config-reader/config-reader-types.h"

#include <httplib/httplib.h>
#include <json/json.hpp>

#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <vector>

namespace loomis
{
   enum class EmbySearchType
   {
      id,
      name,
      path
   };

   struct EmbyItemSeries
   {
      std::string name;
      uint32_t seasonNum{0u};
      uint32_t episodeNum{0u};
   };

   struct EmbyItem
   {
      std::string name;
      std::string id;
      std::string path;
      std::string type;
      EmbyItemSeries series;
      uint64_t runTimeTicks{0u};
   };

   struct EmbyPlaylistItem
   {
      std::string name;
      std::string id;
      std::string playlistId;
   };

   struct EmbyPlaylist
   {
      std::string name;
      std::string id;
      std::vector<EmbyPlaylistItem> items;
   };

   class EmbyApi : public ApiBase
   {
   public:
      EmbyApi(const ServerConfig& serverConfig);
      virtual ~EmbyApi() = default;

      // Returns true if the server is reachable and the API key is valid
      [[nodiscard]] bool GetValid() override;
      [[nodiscard]] std::optional<std::string> GetServerReportedName() override;
      [[nodiscard]] std::optional<std::string> GetLibraryId(std::string_view libraryName);

      std::optional<EmbyItem> GetItem(EmbySearchType type, std::string_view name, std::list<std::pair<std::string_view, std::string_view>> extraSearchArgs = {});
      [[nodiscard]] std::optional<std::string> GetItemIdFromPath(std::string_view path);

      [[nodiscard]] bool GetPlaylistExists(std::string_view name);
      [[nodiscard]] std::optional<EmbyPlaylist> GetPlaylist(std::string_view name);
      void CreatePlaylist(std::string_view name, const std::vector<std::string>& itemIds);
      bool AddPlaylistItems(std::string_view playlistId, const std::vector<std::string>& addIds);
      bool RemovePlaylistItems(std::string_view playlistId, const std::vector<std::string>& removeIds);
      bool MovePlaylistItem(std::string_view playlistId, std::string_view itemId, uint32_t index);

      // Tell Emby to scan the passed in library
      void SetLibraryScan(std::string_view libraryId);

   private:
      std::string BuildApiPath(std::string_view path) const;
      void AddApiParam(std::string& url, const std::list<std::pair<std::string_view, std::string_view>>& params) const;

      std::string_view GetSearchTypeStr(EmbySearchType type);

      std::string BuildCommaSeparatedList(const std::vector<std::string>& list);

      httplib::Client client_;
      httplib::Headers emptyHeaders_;
      httplib::Headers jsonHeaders_{{{"accept", "application/json"}}};
   };
}