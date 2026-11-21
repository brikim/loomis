#include "tmdb-client.h"

#include "services/service-utils.h"

#include <algorithm>
#include <glaze/glaze.hpp>
#include <unordered_set>
#include <utility>

namespace loomis
{
   namespace
   {
      constexpr std::string_view TMDB_API_URL{"https://api.themoviedb.org/3"};
      constexpr int32_t FIRST_TMDB_PAGE{1};
      constexpr int32_t MAX_TMDB_PAGE{500};
      constexpr std::size_t TMDB_FIXED_QUERY_PARAM_COUNT{2};
      constexpr int64_t INVALID_TMDB_ID{0};

      struct TmdbMediaResult
      {
         int64_t id{INVALID_TMDB_ID};
         std::optional<std::string> mediaType;

         struct glaze
         {
            // clang-format off
            static constexpr auto value = glz::object(
               "id", &TmdbMediaResult::id,
               "media_type", &TmdbMediaResult::mediaType
            );
            // clang-format on
         };
      };

      struct TmdbMediaPage
      {
         std::vector<TmdbMediaResult> results;
         int32_t totalPages{FIRST_TMDB_PAGE};

         struct glaze
         {
            // clang-format off
            static constexpr auto value = glz::object(
               "results", &TmdbMediaPage::results,
               "total_pages", &TmdbMediaPage::totalPages
            );
            // clang-format on
         };
      };

   } // namespace

   TmdbClient::TmdbClient(std::string apiKey, ServiceLogger logger, bool dryRun)
      : apiKey_(std::move(apiKey))
      , logger_(logger)
      , dryRunText_(GetDryRunText(dryRun))
      , httpClient_(TMDB_API_URL)
   {
   }

   std::optional<std::vector<int64_t>> TmdbClient::GetMediaItems(
      std::string_view path,
      std::size_t maxItems,
      const TmdbQueryParams& params)
   {
      std::vector<int64_t> items;
      if (maxItems == 0)
         return items;
      std::unordered_set<int64_t> seenItems;

      int32_t totalPages{FIRST_TMDB_PAGE};
      int32_t currentPage{FIRST_TMDB_PAGE};
      auto pageParams = params;
      pageParams.emplace_back("page", std::string{});
      // Returns early when maxItems is reached; MAX_TMDB_PAGE is only a safety cap.
      while (currentPage <= MAX_TMDB_PAGE)
      {
         pageParams.back().second = std::to_string(currentPage);

         const auto response = GetResponse(path, pageParams);
         if (!response)
            return std::nullopt;

         auto pageItems = ParseMediaItems(*response, totalPages);
         if (!pageItems)
            return std::nullopt;

         for (const auto item : *pageItems)
         {
            if (seenItems.emplace(item).second)
               items.emplace_back(item);

            if (items.size() >= maxItems)
               return items;
         }

         if (currentPage >= totalPages)
            return items;

         ++currentPage;
      }

      logger_.LogWarning(
         "{}Reached TMDB's page limit with {} of {} requested media items.",
         dryRunText_,
         items.size(),
         maxItems);

      return items;
   }

   std::optional<std::string> TmdbClient::GetResponse(
      std::string_view path,
      const TmdbQueryParams& params)
   {
      warp::HttpParams requestParams;
      requestParams.reserve(params.size() + TMDB_FIXED_QUERY_PARAM_COUNT);
      requestParams.emplace_back("api_key", apiKey_);
      requestParams.emplace_back("language", "en-US");
      for (const auto& [key, value] : params)
         requestParams.emplace_back(key, value);

      const auto requestPath = httpClient_.BuildHttpParamsPath(
         httpClient_.GetUrl(),
         path,
         requestParams);
      auto response = httpClient_.Get(requestPath, {});
      if (const auto error = httpClient_.GetHttpResponseStr(response); error)
      {
         logger_.LogWarning(
            "{}TMDB request failed for {}: error={}",
            dryRunText_,
            path,
            *error);
         return std::nullopt;
      }

      constexpr int32_t TMDB_HTTP_OK_STATUS{200};
      if (response.status != TMDB_HTTP_OK_STATUS)
      {
         logger_.LogWarning(
            "{}TMDB returned status {} for {}: body={}",
            dryRunText_,
            response.status,
            path,
            response.body);
         return std::nullopt;
      }

      return std::move(response.body);
   }

   std::optional<std::vector<int64_t>> TmdbClient::ParseMediaItems(
      const std::string& responseBody,
      int32_t& totalPages)
   {
      TmdbMediaPage page;
      if (auto ec = glz::read<glz::opts{.error_on_unknown_keys = false}>(page, responseBody);
          ec)
      {
         logger_.LogWarning("{}{} - JSON Parse Error: {}", dryRunText_, __func__, glz::format_error(ec, responseBody));
         return std::nullopt;
      }

      totalPages = std::max(page.totalPages, FIRST_TMDB_PAGE);

      std::vector<int64_t> items;
      items.reserve(page.results.size());
      for (const auto& result : page.results)
      {
         if (result.id == INVALID_TMDB_ID)
            continue;

         if (result.mediaType && *result.mediaType != TMDB_MEDIA_TYPE_MOVIE)
            continue;

         items.push_back(result.id);
      }

      return items;
   }
} // namespace loomis
