#pragma once

#include "api/api-base.h"
#include "api/api-emby-types.h"
#include "config-reader/config-reader-types.h"

#include <httplib/httplib.h>

#include <chrono>
#include <cstdint>
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace loomis
{
   class EmbyApi : public ApiBase
   {
   public:
      EmbyApi(const ServerConfig& serverConfig);
      virtual ~EmbyApi() = default;

      [[nodiscard]] std::optional<std::vector<Task>> GetTaskList() override;

      // Returns true if the server is reachable and the API key is valid
      [[nodiscard]] bool GetValid() override;
      [[nodiscard]] std::optional<std::string> GetServerReportedName() override;
      [[nodiscard]] std::optional<std::string> GetLibraryId(std::string_view libraryName);

      std::optional<EmbyItem> GetItem(EmbySearchType type, std::string_view name, std::list<std::pair<std::string_view, std::string_view>> extraSearchArgs = {});

      [[nodiscard]] bool GetUserExists(std::string_view name);

      [[nodiscard]] bool GetPlaylistExists(std::string_view name);
      [[nodiscard]] std::optional<EmbyPlaylist> GetPlaylist(std::string_view name);
      void CreatePlaylist(std::string_view name, const std::vector<std::string>& itemIds);
      bool AddPlaylistItems(std::string_view playlistId, const std::vector<std::string>& addIds);
      bool RemovePlaylistItems(std::string_view playlistId, const std::vector<std::string>& removeIds);
      bool MovePlaylistItem(std::string_view playlistId, std::string_view itemId, uint32_t index);

      // Tell Emby to scan the passed in library
      void SetLibraryScan(std::string_view libraryId);

      [[nodiscard]] bool GetPathMapEmpty() const;
      [[nodiscard]] std::optional<std::string> GetIdFromPathMap(const std::string& path);

   private:
      void BuildPathMap(const std::chrono::system_clock::time_point& time);
      void RunPathMapQuickCheck();
      void RunPathMapFullUpdate();

      bool HasLibraryChanged();
      std::string BuildApiPath(std::string_view path) const;

      std::string_view GetSearchTypeStr(EmbySearchType type);

      httplib::Client client_;
      httplib::Headers emptyHeaders_;
      httplib::Headers jsonHeaders_{{{"accept", "application/json"}}};

      std::string lastSyncTimestamp_;
      std::chrono::system_clock::time_point pathMapUpdateTime_;
      EmbyPathMap pathMap_;
      EmbyPathMap workingPathMap_;

      mutable std::mutex taskLock_;
   };
}