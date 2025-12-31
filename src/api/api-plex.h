#pragma once

#include "api/api-base.h"
#include "config-reader/config-reader-types.h"

#include <httplib/httplib.h>
#include <pugixml/src/pugixml.hpp>

#include <list>
#include <optional>
#include <string>

namespace loomis
{
   struct PlexCollectionItem
   {
      std::string title;
      std::vector<std::string> paths;
   };

   struct PlexCollection
   {
      std::string name;
      std::vector<PlexCollectionItem> items;
   };

   class PlexApi : public ApiBase
   {
   public:
      PlexApi(const ServerConfig& serverConfig);
      virtual ~PlexApi() = default;

      // Returns true if the server is reachable and the API key is valid
      [[nodiscard]] bool GetValid() override;
      [[nodiscard]] std::optional<std::string> GetServerReportedName() override;
      [[nodiscard]] std::optional<std::string> GetLibraryId(std::string_view libraryName);

      // Returns if the collection in the library is valid on this server
      [[nodiscard]] bool GetCollectionValid(std::string_view library, std::string_view collection);

      // Returns the collection information for the collection in the library
      [[nodiscard]] std::optional<PlexCollection> GetCollection(std::string_view library, std::string_view collection);

      // Tell Plex to scan the passed in library
      void SetLibraryScan(std::string_view libraryId);

   private:
      std::string BuildApiPath(std::string_view path);
      const pugi::xml_node* GetCollectionNode(std::string_view library, std::string_view collection);

      httplib::Client client_;
      httplib::Headers headers_;

      pugi::xml_document collectionDoc_;
   };
}