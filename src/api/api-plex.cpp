#include "api-plex.h"

#include "api/api-utils.h"
#include "logger/logger.h"
#include "logger/log-utils.h"
#include "types.h"

#include <format>
#include <ranges>

namespace loomis
{
   namespace
   {
      const std::string API_BASE{""};
      const std::string API_TOKEN_NAME("X-Plex-Token");

      const std::string API_SERVERS{"/servers"};
      const std::string API_LIBRARIES{"/library/sections/"};
      const std::string API_LIBRARY_DATA{"/library/metadata/"};
      const std::string API_SEARCH{"/hubs/search"};

      constexpr std::string_view ELEM_MEDIA_CONTAINER{"MediaContainer"};
      constexpr std::string_view ELEM_MEDIA{"Media"};
      constexpr std::string_view ELEM_VIDEO{"Video"};

      constexpr std::string_view ATTR_NAME{"name"};
      constexpr std::string_view ATTR_KEY{"key"};
      constexpr std::string_view ATTR_TITLE{"title"};
      constexpr std::string_view ATTR_FILE{"file"};
   }

   PlexApi::PlexApi(const ServerConfig& serverConfig)
      : ApiBase(serverConfig.server_name, serverConfig.url, serverConfig.api_key, "PlexApi", utils::ANSI_CODE_PLEX)
      , client_(GetUrl())
      , mediaPath_(serverConfig.media_path)
   {
      constexpr time_t timeoutSec{5};
      client_.set_connection_timeout(timeoutSec);
   }

   std::string_view PlexApi::GetApiBase() const
   {
      return API_BASE;
   }

   std::string_view PlexApi::GetApiTokenName() const
   {
      return API_TOKEN_NAME;
   }

   bool PlexApi::GetValid()
   {
      auto res = client_.Get(BuildApiPath(API_SERVERS), headers_);
      return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
   }

   const std::string& PlexApi::GetMediaPath() const
   {
      return mediaPath_;
   }

   std::optional<PlexSearchResults> PlexApi::SearchItem(std::string_view name)
   {
      const auto apiUrl = BuildApiParamsPath(API_SEARCH, {
         {"query", name}
      });
      auto res = client_.Get(apiUrl, headers_);

      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      pugi::xml_document doc;
      if (doc.load_buffer(res->body.data(), res->body.size()).status != pugi::status_ok)
      {
         LogWarning("{} - Malformed XML reply received", std::string(__func__));
         return std::nullopt;
      }

      PlexSearchResults returnResults;

      // Faster XPath by providing the direct path
      pugi::xpath_node_set videos = doc.select_nodes("/MediaContainer/Hub[@type='episode' or @type='movie']/Video");

      for (pugi::xpath_node node : videos)
      {
         pugi::xml_node video = node.node();
         auto& item = returnResults.items.emplace_back();

         item.libraryName = video.attribute("librarySectionTitle").as_string();
         item.title = video.attribute("title").as_string();
         item.ratingKey = video.attribute("ratingKey").as_string();
         item.durationMs = video.attribute("duration").as_llong();

         if (pugi::xml_node part = video.child("Media").child("Part"))
         {
            item.path = part.attribute("file").as_string();
         }
      }

      return returnResults;
   }

   std::optional<std::string> PlexApi::GetServerReportedName()
   {
      auto res = client_.Get(BuildApiPath(API_SERVERS), headers_);

      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      pugi::xml_document doc;
      if (doc.load_buffer(res->body.data(), res->body.size()).status != pugi::status_ok)
      {
         LogWarning("{} - Malformed XML reply received", std::string(__func__));
         return std::nullopt;
      }

      pugi::xpath_node serverNode = doc.select_node("//Server[@name]");

      if (!serverNode)
      {
         LogWarning("{} - No Server element with a name attribute found", __func__);
         return std::nullopt;
      }

      return serverNode.node().attribute(ATTR_NAME).as_string();
   }

   std::optional<std::string> PlexApi::GetLibraryId(std::string_view libraryName)
   {
      auto res = client_.Get(BuildApiPath(API_LIBRARIES), headers_);

      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      pugi::xml_document doc;
      if (doc.load_buffer(res->body.data(), res->body.size()).status != pugi::status_ok)
      {
         return std::nullopt;
      }

      std::string query = std::format("//{}[@{}='{}']", "Directory", ATTR_TITLE, libraryName);
      pugi::xpath_node libraryNode = doc.select_node(query.c_str());

      if (!libraryNode)
      {
         return std::nullopt;
      }

      std::string key = libraryNode.node().attribute(ATTR_KEY).as_string();

      return key.empty() ? std::nullopt : std::make_optional(key);
   }

   std::optional<std::string> PlexApi::GetItemPath(int32_t id)
   {
      auto pathName = API_LIBRARY_DATA + std::to_string(id);
      auto res = client_.Get(BuildApiPath(pathName), headers_);

      if (!IsHttpSuccess(__func__, res))
      {
         return std::nullopt;
      }

      pugi::xml_document doc;
      auto parse_result = doc.load_buffer(res->body.data(), res->body.size());

      if (parse_result.status != pugi::status_ok)
      {
         return std::nullopt;
      }

      auto container = doc.child(ELEM_MEDIA_CONTAINER);
      if (!container) return std::nullopt;

      auto videoNode = container.child(ELEM_VIDEO);
      if (!videoNode) return std::nullopt;

      auto partNode = videoNode.child("Media").child("Part");

      std::string filePath = partNode.attribute("file").as_string();
      if (filePath.empty())
      {
         return std::nullopt;
      }

      return filePath;
   }

   std::unordered_map<int32_t, std::string> PlexApi::GetItemsPaths(const std::vector<int32_t>& ids)
   {
      auto res = client_.Get(BuildApiPath(API_LIBRARY_DATA + BuildCommaSeparatedList(ids)), headers_);
      if (!IsHttpSuccess(__func__, res)) return {};

      pugi::xml_document doc;
      auto parse_result = doc.load_buffer(res->body.data(), res->body.size());

      if (parse_result.status != pugi::status_ok) return {};

      auto container = doc.child(ELEM_MEDIA_CONTAINER);
      if (!container) return {};

      std::unordered_map<int32_t, std::string> results;
      results.reserve(ids.size());

      for (auto videoNode : container.children(ELEM_VIDEO.data()))
      {
         int32_t ratingKey = videoNode.attribute("ratingKey").as_int();
         std::string filePath = videoNode.child("Media").child("Part").attribute("file").as_string();

         if (ratingKey != 0 && !filePath.empty())
         {
            // Use move to transfer the string into the map efficiently
            results.emplace(ratingKey, std::move(filePath));
         }
      }

      return results;
   }

   void PlexApi::SetLibraryScan(std::string_view libraryId)
   {
      auto apiUrl = BuildApiPath(std::format("{}{}/refresh", API_LIBRARIES, libraryId));
      auto res = client_.Get(apiUrl, headers_);
      IsHttpSuccess(__func__, res);
   }

   pugi::xml_node PlexApi::GetCollectionNode(std::string_view library, std::string_view collection)
   {
      auto libraryId = GetLibraryId(library);
      if (!libraryId) return {}; // Returns a "null" node

      std::string apiUrl = BuildApiPath(std::format("{}{}/all", API_LIBRARIES, *libraryId));
      apiUrl += std::format("&type={}&title={}",
                            static_cast<int>(PlexSearchTypes::collection),
                            collection);

      auto res = client_.Get(apiUrl, headers_);

      if (!IsHttpSuccess(__func__, res)) return {};

      if (collectionDoc_.load_buffer(res->body.data(), res->body.size()).status != pugi::status_ok)
      {
         return {};
      }

      std::string query = std::format("//{}[@{}='{}']", "Directory", ATTR_TITLE, collection);
      pugi::xpath_node match = collectionDoc_.select_node(query.c_str());

      return match.node();
   }

   bool PlexApi::GetCollectionValid(std::string_view library, std::string_view collection)
   {
      return !GetCollectionNode(library, collection).empty();
   }

   std::optional<PlexCollection> PlexApi::GetCollection(std::string_view library, std::string_view collectionName)
   {
      auto node = GetCollectionNode(library, collectionName);
      if (node.empty()) return std::nullopt;

      auto key = node.attribute(ATTR_KEY).as_string();
      if (std::string_view(key).empty()) return std::nullopt;

      auto res = client_.Get(BuildApiPath(key), headers_);
      if (!IsHttpSuccess(__func__, res)) return std::nullopt;

      pugi::xml_document doc;
      if (doc.load_buffer(res->body.data(), res->body.size()).status != pugi::status_ok) return std::nullopt;

      auto container = doc.child(ELEM_MEDIA_CONTAINER);
      if (!container) return std::nullopt;

      PlexCollection collection;
      collection.name = collectionName;

      for (auto itemNode : container.children())
      {
         // Skip items without media info
         auto mediaNode = itemNode.child(ELEM_MEDIA);
         if (!mediaNode) continue;

         auto& item = collection.items.emplace_back();
         item.title = itemNode.attribute(ATTR_TITLE).as_string();

         // 4. Extract paths from Media -> Part hierarchy
         for (auto partNode : itemNode.select_nodes("Media/Part"))
         {
            if (auto path = partNode.node().attribute("file").as_string(); *path)
            {
               item.paths.emplace_back(path);
            }
         }
      }

      return collection;
   }

   bool PlexApi::SetPlayed(std::string_view name, std::string_view path, int32_t percent)
   {
      auto results = SearchItem(name);
      if (!results)
      {
         LogError("{} - Search for item {} failed", __func__, name);
         return false;
      }

      auto iter = std::ranges::find_if(results->items, [&](const auto& item) {
         return item.path == path;
      });

      if (iter == results->items.end())
      {
         LogError("{} - Item {} with path {} not found in search results", __func__, name, path);
         return false;
      }

      auto newLocationMs = static_cast<int64_t>((iter->durationMs * percent) / 100);
      const auto apiUrl = BuildApiParamsPath("/:/progress", {
         {"identifier", "com.plexapp.plugins.library"},
         {"key", iter->ratingKey},
         {"time", std::to_string(newLocationMs)},
         {"state", "stopped"} // 'stopped' commits the time to the database
      });

      auto res = client_.Get(apiUrl, headers_);
      if (!IsHttpSuccess(__func__, res))
      {
         LogError("{} - Failed to mark {} as played {}%", __func__, name, percent);
         return false;
      }

      return true;
   }

   bool PlexApi::SetWatched(std::string_view name, std::string_view path)
   {
      auto results = SearchItem(name);
      if (!results)
      {
         LogError("{} - Search for item {} failed", __func__, name);
         return false;
      }

      auto iter = std::ranges::find_if(results->items, [&](const auto& item) {
         return item.path == path;
      });

      if (iter == results->items.end())
      {
         LogError("{} - Item {} with path {} not found in search results", __func__, name, path);
         return false;
      }

      const auto apiUrl = BuildApiParamsPath("/:/scrobble", {
         {"identifier", "com.plexapp.plugins.library"},
         {"key", iter->ratingKey}
      });

      auto res = client_.Get(apiUrl, headers_);
      if (!IsHttpSuccess(__func__, res)) return false;
      {
         LogError("{} - Failed to mark {} as watched", __func__, name);
         return false;
      }

      return true;
   }
}