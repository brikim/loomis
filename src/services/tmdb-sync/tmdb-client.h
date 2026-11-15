#pragma once

#include "services/service-logger.h"
#include "services/tmdb-sync/tmdb-media-item.h"

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
   using TmdbQueryParams = std::vector<std::pair<std::string, std::string>>;

   class TmdbClient
   {
   public:
      TmdbClient(std::string apiKey, ServiceLogger logger, bool dryRun);

      [[nodiscard]] std::optional<std::vector<TmdbMediaItem>> GetMediaItems(
         std::string_view path,
         TmdbMediaType defaultMediaType,
         std::size_t maxItems,
         const TmdbQueryParams& params = {});

   private:
      [[nodiscard]] std::optional<std::string> GetResponse(
         std::string_view path,
         const TmdbQueryParams& params);

      [[nodiscard]] std::optional<std::vector<TmdbMediaItem>> ParseMediaItems(
         const std::string& responseBody,
         TmdbMediaType defaultMediaType,
         int32_t& totalPages);

      std::string apiKey_;
      ServiceLogger logger_;
      std::string dryRunText_;
      warp::HttpClient httpClient_;
   };
} // namespace loomis
