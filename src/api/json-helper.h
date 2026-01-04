#pragma once

#include "logger/logger.h"
#include "logger/log-utils.h"

#include <json/json.hpp>

#include <optional>
#include <source_location>
#include <string>

namespace loomis
{
   inline std::optional<nlohmann::json> JsonSafeParse(std::string_view rawJson, const std::source_location location = std::source_location::current())
   {
      if (rawJson.empty()) return std::nullopt;

      try
      {
         return nlohmann::json::parse(rawJson);
      }
      catch (std::exception& e)
      {
         Logger::Instance().Warning("JSON parse {} {} {} {}",
                                    utils::GetTag("file", location.file_name()),
                                    utils::GetTag("function_name", location.function_name()),
                                    utils::GetTag("line_num", location.line()),
                                    utils::GetTag("error", e.what()));
         return std::nullopt;
      }
   }

   template <typename T>
   inline std::optional<T> JsonSafeGet(const nlohmann::json& data, std::string_view key, const std::source_location location = std::source_location::current())
   {
      try
      {
         if (data.contains(key) && !data[key].is_null())
         {
            return data[key].get<T>();
         }
      }
      catch (std::exception& e)
      {
         Logger::Instance().Warning("JSON get {} {} {} {} {}",
                                    utils::GetTag("file", location.file_name()),
                                    utils::GetTag("function_name", location.function_name()),
                                    utils::GetTag("line_num", location.line()),
                                    utils::GetTag("key", key),
                                    utils::GetTag("error", e.what()));
      }
      return std::nullopt;
   }

   template <typename T>
   inline std::optional<T> JsonSafeGet(const nlohmann::json& data, const nlohmann::json::json_pointer& ptr, const std::source_location location = std::source_location::current())
   {
      try
      {
         if (data.contains(ptr) && !data[ptr].is_null())
         {
            return data[ptr].get<T>();
         }
      }
      catch (std::exception& e)
      {
         Logger::Instance().Warning("JSON get pointer {} {} {} {} {}",
                                    utils::GetTag("file", location.file_name()),
                                    utils::GetTag("function_name", location.function_name()),
                                    utils::GetTag("line_num", location.line()),
                                    utils::GetTag("ptr", ptr.to_string()),
                                    utils::GetTag("error", e.what()));
      }
      return std::nullopt;
   }
}