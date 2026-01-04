#pragma once

#include <cstdint>
#include <string>

namespace loomis
{
   struct JsonTautulliMonitorInfo
   {
      int32_t movie_watched_percent{0};
   };

   struct JsonTautulliServerInfo
   {
      std::string pms_name;
   };

   struct JsonUserInfo
   {
      std::string username;
      int32_t user_id{0};
      std::string friendly_name;
   };

   struct JsonTautulliHistoryItem
   {
      std::string title;
      std::string full_title;
      int32_t rating_key{0};
      int64_t stopped{0};
      int32_t percent_complete{0};
   };

   struct JsonTautulliHistoryData
   {
      std::vector<JsonTautulliHistoryItem> data;
   };

   template <typename T>
   struct JsonTautulliResponse
   {
      struct InternalData
      {
         std::string result; // "success" or "error"
         T data;              // This is the actual object/vector you want
      };

      InternalData response;
   };
}