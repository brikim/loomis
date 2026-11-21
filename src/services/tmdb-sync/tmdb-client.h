#pragma once

#include "services/service-logger.h"

#include <warp/http/http-client.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace loomis
{
   inline constexpr std::string_view TMDB_MEDIA_TYPE_MOVIE{"movie"};

   using TmdbQueryParams = std::vector<std::pair<std::string, std::string>>;

   class TmdbClient
   {
   public:
      TmdbClient(std::string apiKey, ServiceLogger logger, bool dryRun);

      [[nodiscard]] std::optional<std::vector<int64_t>> GetMediaItems(
         std::string_view path,
         std::size_t maxItems,
         const TmdbQueryParams& params = {});

   private:
      [[nodiscard]] std::optional<std::string> GetResponse(
         std::string_view path,
         const TmdbQueryParams& params);

      [[nodiscard]] std::optional<std::vector<int64_t>> ParseMediaItems(
         const std::string& responseBody,
         int32_t& totalPages);

      std::string apiKey_;
      ServiceLogger logger_;
      std::string_view dryRunText_;
      warp::HttpClient httpClient_;
   };
} // namespace loomis
