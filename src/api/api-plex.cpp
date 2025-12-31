#include "api-plex.h"

#include "logger/logger.h"
#include "logger/log-utils.h"
#include "types.h"

#include <format>
#include <ranges>

namespace loomis
{
   inline const std::string PLEX_API_BASE{""};
   inline const std::string PLEX_API_SERVERS{"/servers"};
   inline const std::string PLEX_API_LIBRARIES{"/library/sections/"};

   inline constexpr std::string_view PLEX_ELEM_MEDIA_CONTAINER{"MediaContainer"};
   inline constexpr std::string_view PLEX_ELEM_MEDIA{"Media"};

   inline constexpr std::string_view PLEX_ATTR_NAME{"name"};
   inline constexpr std::string_view PLEX_ATTR_KEY{"key"};
   inline constexpr std::string_view PLEX_ATTR_TITLE{"title"};
   inline constexpr std::string_view PLEX_ATTR_FILE{"file"};

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
      : ApiBase(serverConfig.name, serverConfig.main, "PlexApi", utils::ANSI_CODE_PLEX)
      , client_(GetUrl())
   {
      constexpr time_t timeoutSec{5};
      client_.set_connection_timeout(timeoutSec);
   }

   std::string PlexApi::BuildApiPath(std::string_view path)
   {
      return std::format("{}{}?X-Plex-Token={}", PLEX_API_BASE, path, GetApiKey());
   }

   bool PlexApi::GetValid()
   {
      auto res = client_.Get(BuildApiPath(PLEX_API_SERVERS), headers_);
      return res.error() == httplib::Error::Success && res.value().status < VALID_HTTP_RESPONSE_MAX;
   }

   std::optional<std::string> PlexApi::GetServerReportedName()
   {
      httplib::Headers header;
      if (auto res = client_.Get(BuildApiPath(PLEX_API_SERVERS), header);
          res.error() == httplib::Error::Success)
      {
         pugi::xml_document data;
         if (data.load_buffer(res.value().body.c_str(), res.value().body.size()).status == pugi::status_ok
             && data.child(PLEX_ELEM_MEDIA_CONTAINER)
             && data.child(PLEX_ELEM_MEDIA_CONTAINER).first_child()
             && data.child(PLEX_ELEM_MEDIA_CONTAINER).first_child().attribute(PLEX_ATTR_NAME))
         {
            return data.child(PLEX_ELEM_MEDIA_CONTAINER).first_child().attribute(PLEX_ATTR_NAME).as_string();
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
      if (auto res = client_.Get(BuildApiPath(PLEX_API_LIBRARIES), headers_);
          res.error() == httplib::Error::Success)
      {
         pugi::xml_document data;
         if (data.load_buffer(res.value().body.c_str(), res.value().body.size()).status == pugi::status_ok
             && data.child(PLEX_ELEM_MEDIA_CONTAINER))
         {
            for (const auto& library : data.child(PLEX_ELEM_MEDIA_CONTAINER))
            {
               if (library
                   && library.attribute(PLEX_ATTR_TITLE)
                   && library.attribute(PLEX_ATTR_TITLE).as_string() == libraryName
                   && library.attribute(PLEX_ATTR_KEY))
               {
                  return library.attribute(PLEX_ATTR_KEY).as_string();
               }
            }
         }
      }
      return std::nullopt;
   }

   void PlexApi::SetLibraryScan(std::string_view libraryId)
   {
      auto apiUrl = BuildApiPath(std::format("{}{}/refresh", PLEX_API_LIBRARIES, libraryId));
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

      auto apiUrl = BuildApiPath(std::format("{}{}/all", PLEX_API_LIBRARIES, libraryId.value()));
      apiUrl.append(std::format("&type={}", static_cast<int>(PlexSearchTypes::collection), collection));
      if (auto res = client_.Get(apiUrl, headers_);
          res.error() == httplib::Error::Success)
      {
         if (collectionDoc_.load_buffer(res.value().body.c_str(), res.value().body.size()).status == pugi::status_ok
             && collectionDoc_.child(PLEX_ELEM_MEDIA_CONTAINER))
         {
            for (const auto& plexCollection : collectionDoc_.child(PLEX_ELEM_MEDIA_CONTAINER))
            {
               if (plexCollection.attribute(PLEX_ATTR_TITLE)
                   && collection == plexCollection.attribute(PLEX_ATTR_TITLE).as_string())
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

   std::optional<PlexCollection> PlexApi::GetCollection(std::string_view library, std::string_view collection)
   {
      if (auto* node{GetCollectionNode(library, collection)};
          node != nullptr
          && node->attribute(PLEX_ATTR_KEY))
      {
         auto apiUrl = BuildApiPath(node->attribute(PLEX_ATTR_KEY).as_string());
         if (auto res = client_.Get(apiUrl, headers_);
             res.error() == httplib::Error::Success)
         {
            pugi::xml_document data;
            if (data.load_buffer(res.value().body.c_str(), res.value().body.size()).status == pugi::status_ok
                && data.child(PLEX_ELEM_MEDIA_CONTAINER))
            {
               PlexCollection  returnCollection;
               returnCollection.name = collection;

               for (const auto& itemNode : data.child(PLEX_ELEM_MEDIA_CONTAINER))
               {
                  if (itemNode.attribute(PLEX_ATTR_TITLE)
                      && itemNode.child(PLEX_ELEM_MEDIA))
                  {
                     auto& collectionItem{returnCollection.items.emplace_back()};
                     collectionItem.title = itemNode.attribute(PLEX_ATTR_TITLE).as_string();

                     for (const auto& mediaNode : itemNode.child(PLEX_ELEM_MEDIA))
                     {
                        if (mediaNode.attribute(PLEX_ATTR_FILE))
                        {
                           collectionItem.paths.emplace_back(mediaNode.attribute(PLEX_ATTR_FILE).as_string());
                        }
                     }
                  }
               }

               return returnCollection;
            }
         }
      }

      return std::nullopt;
   }
}