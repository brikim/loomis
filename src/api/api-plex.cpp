#include "api-plex.h"

#include "logger/logger.h"
#include "logger/log-utils.h"
#include "types.h"

#include <format>
#include <ranges>

namespace loomis
{
   static const std::string API_BASE{""};
   static const std::string API_SERVERS{"/servers"};
   static const std::string API_LIBRARIES{"/library/sections/"};

   static constexpr std::string_view ELEM_MEDIA_CONTAINER{"MediaContainer"};
   static constexpr std::string_view ELEM_MEDIA{"Media"};

   static constexpr std::string_view ATTR_NAME{"name"};
   static constexpr std::string_view ATTR_KEY{"key"};
   static constexpr std::string_view ATTR_TITLE{"title"};
   static constexpr std::string_view ATTR_FILE{"file"};

   enum class PlexSearchTypes
   {
      movie = 1,
      show = 2,
      season = 3,
      episode = 4,
      trailer = 5,
      comic = 6,
      person = 7,
      artist = 8,
      album = 9,
      track = 10,
      picture = 11,
      clip = 12,
      photo = 13,
      photoalbum = 14,
      playlist = 15,
      playlistFolder = 16,
      collection = 18,
      optimizedVersion = 42,
      userPlaylistItem = 1001,
   };

   PlexApi::PlexApi(const ServerConfig& serverConfig)
      : ApiBase(serverConfig, "PlexApi", utils::ANSI_CODE_PLEX)
      , client_(GetUrl())
   {
      constexpr time_t timeoutSec{5};
      client_.set_connection_timeout(timeoutSec);
   }

   std::string PlexApi::BuildApiPath(std::string_view path)
   {
      return std::format("{}{}?X-Plex-Token={}", API_BASE, path, GetApiKey());
   }

   bool PlexApi::GetValid()
   {
      auto res = client_.Get(BuildApiPath(API_SERVERS), headers_);
      return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
   }

   std::optional<std::string> PlexApi::GetServerReportedName()
   {
      httplib::Headers header;
      if (auto res = client_.Get(BuildApiPath(API_SERVERS), header);
          res.error() == httplib::Error::Success)
      {
         pugi::xml_document data;
         if (data.load_buffer(res.value().body.c_str(), res.value().body.size()).status == pugi::status_ok
             && data.child(ELEM_MEDIA_CONTAINER)
             && data.child(ELEM_MEDIA_CONTAINER).first_child()
             && data.child(ELEM_MEDIA_CONTAINER).first_child().attribute(ATTR_NAME))
         {
            return data.child(ELEM_MEDIA_CONTAINER).first_child().attribute(ATTR_NAME).as_string();
         }
         else
         {
            LogWarning("GetServerReportedName malformed xml reply received");
         }
      }
      return std::nullopt;
   }

   std::optional<std::string> PlexApi::GetLibraryId(std::string_view libraryName)
   {
      if (auto res = client_.Get(BuildApiPath(API_LIBRARIES), headers_);
          res.error() == httplib::Error::Success)
      {
         pugi::xml_document data;
         if (data.load_buffer(res.value().body.c_str(), res.value().body.size()).status == pugi::status_ok
             && data.child(ELEM_MEDIA_CONTAINER))
         {
            for (const auto& library : data.child(ELEM_MEDIA_CONTAINER))
            {
               if (library
                   && library.attribute(ATTR_TITLE)
                   && library.attribute(ATTR_TITLE).as_string() == libraryName
                   && library.attribute(ATTR_KEY))
               {
                  return library.attribute(ATTR_KEY).as_string();
               }
            }
         }
      }
      return std::nullopt;
   }

   void PlexApi::SetLibraryScan(std::string_view libraryId)
   {
      auto apiUrl = BuildApiPath(std::format("{}{}/refresh", API_LIBRARIES, libraryId));
      if (auto res = client_.Get(apiUrl, headers_);
          res.error() != httplib::Error::Success
          || res.value().status >= VALID_HTTP_RESPONSE_MAX)
      {
         LogWarning(std::format("Library Scan {}",
                                utils::GetTag("error", res.error() != httplib::Error::Success
                                              ? std::to_string(static_cast<int>(res.error()))
                                              : std::format("{} - {}", res.value().reason, res.value().body))
         ));
      }
   }

   const pugi::xml_node* PlexApi::GetCollectionNode(std::string_view library, std::string_view collection)
   {
      auto libraryId{GetLibraryId(library)};
      if (!libraryId.has_value())
      {
         return nullptr;
      }

      auto apiUrl = BuildApiPath(std::format("{}{}/all", API_LIBRARIES, libraryId.value()));
      apiUrl.append(std::format("&type={}", static_cast<int>(PlexSearchTypes::collection), collection));
      if (auto res = client_.Get(apiUrl, headers_);
          res.error() == httplib::Error::Success)
      {
         if (collectionDoc_.load_buffer(res.value().body.c_str(), res.value().body.size()).status == pugi::status_ok
             && collectionDoc_.child(ELEM_MEDIA_CONTAINER))
         {
            for (const auto& plexCollection : collectionDoc_.child(ELEM_MEDIA_CONTAINER))
            {
               if (plexCollection.attribute(ATTR_TITLE)
                   && collection == plexCollection.attribute(ATTR_TITLE).as_string())
               {
                  return &plexCollection;
               }
            }
         }
      }
      return nullptr;
   }

   bool PlexApi::GetCollectionValid(std::string_view library, std::string_view collection)
   {
      return GetCollectionNode(library, collection) != nullptr;
   }

   const PlexCollection& PlexApi::GetCollection(std::string_view library, std::string_view collection)
   {
      // Clear out the collection
      collection_ = {};

      if (auto* node{GetCollectionNode(library, collection)};
          node != nullptr
          && node->attribute(ATTR_KEY))
      {
         auto apiUrl = BuildApiPath(node->attribute(ATTR_KEY).as_string());
         if (auto res = client_.Get(apiUrl, headers_);
             res.error() == httplib::Error::Success)
         {
            pugi::xml_document data;
            if (data.load_buffer(res.value().body.c_str(), res.value().body.size()).status == pugi::status_ok
                && data.child(ELEM_MEDIA_CONTAINER))
            {
               collection_.valid = true;
               collection_.name = collection;

               for (const auto& itemNode : data.child(ELEM_MEDIA_CONTAINER))
               {
                  if (itemNode.attribute(ATTR_TITLE)
                      && itemNode.child(ELEM_MEDIA))
                  {
                     auto& collectionItem{collection_.items.emplace_back()};
                     collectionItem.title = itemNode.attribute(ATTR_TITLE).as_string();

                     for (const auto& mediaNode : itemNode.child(ELEM_MEDIA))
                     {
                        if (mediaNode.attribute(ATTR_FILE))
                        {
                           collectionItem.paths.emplace_back(mediaNode.attribute(ATTR_FILE).as_string());
                        }
                     }
                  }
               }
            }
         }
      }

      return collection_;
   }
}