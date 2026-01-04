#pragma once

#include "api/api-base.h"
#include "api/api-plex-types.h"
#include "config-reader/config-reader-types.h"

#include <httplib/httplib.h>
#include <pugixml/src/pugixml.hpp>

#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>

namespace loomis
{
   class PlexApi : public ApiBase
   {
   public:
      PlexApi(const ServerConfig& serverConfig);
      virtual ~PlexApi() = default;

      // Returns true if the server is reachable and the API key is valid
      [[nodiscard]] bool GetValid() override;
      [[nodiscard]] std::optional<std::string> GetServerReportedName() override;
      [[nodiscard]] std::optional<std::string> GetLibraryId(std::string_view libraryName);
      [[nodiscard]] std::optional<std::string> GetItemPath(int32_t id);
      [[nodiscard]] std::unordered_map<int32_t, std::string> GetItemsPaths(const std::vector<int32_t>& ids);

      // Returns if the collection in the library is valid on this server
      [[nodiscard]] bool GetCollectionValid(std::string_view library, std::string_view collection);

      // Returns the collection information for the collection in the library
      [[nodiscard]] std::optional<PlexCollection> GetCollection(std::string_view library, std::string_view collectionName);

      // Tell Plex to scan the passed in library
      void SetLibraryScan(std::string_view libraryId);

   private:
      std::string BuildApiPath(std::string_view path);
      pugi::xml_node GetCollectionNode(std::string_view library, std::string_view collection);

      httplib::Client client_;
      httplib::Headers headers_;

      pugi::xml_document collectionDoc_;
   };
}