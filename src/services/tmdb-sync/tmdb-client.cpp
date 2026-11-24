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
      constexpr std::size_t TMDB_FIXED_QUERY_PARAM_COUNT{3};
      constexpr int64_t INVALID_TMDB_ID{0};

      struct TmdbMediaResult
      {
         int64_t id{INVALID_TMDB_ID};
         std::optional<std::string_view> mediaType;

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
      items.reserve(maxItems);

      if (maxItems == 0)
         return items;

      std::unordered_set<int64_t> seenItems;
      seenItems.reserve(maxItems);

      int32_t currentPage{FIRST_TMDB_PAGE};

      // Returns early when maxItems is reached; MAX_TMDB_PAGE is only a safety cap.
      while (currentPage <= MAX_TMDB_PAGE)
      {
         const auto response = GetResponse(path, params, currentPage);
         if (!response)
            return std::nullopt;

         auto pageResult = ParseMediaItems(*response);
         if (!pageResult)
            return std::nullopt;

         for (const auto item : pageResult->items)
         {
            if (seenItems.emplace(item).second)
               items.emplace_back(item);

            if (items.size() >= maxItems)
               return items;
         }

         if (pageResult->items.empty() || currentPage >= pageResult->totalPages)
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
      const TmdbQueryParams& params,
      int32_t page)
   {
      const auto pageStr = std::to_string(page);
      warp::HttpParams requestParams;
      requestParams.reserve(params.size() + TMDB_FIXED_QUERY_PARAM_COUNT);
      requestParams.emplace_back("api_key", apiKey_);
      requestParams.emplace_back("language", "en-US");
      requestParams.emplace_back("page", pageStr);
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

   std::optional<TmdbPageResult> TmdbClient::ParseMediaItems(
      std::string_view responseBody)
   {
      TmdbMediaPage page;
      if (auto ec = glz::read<glz::opts{.error_on_unknown_keys = false}>(page, responseBody);
          ec)
      {
         logger_.LogWarning("{}{} - JSON Parse Error: {}", dryRunText_, __func__, glz::format_error(ec, responseBody));
         return std::nullopt;
      }

      TmdbPageResult result;
      result.totalPages = std::max(page.totalPages, FIRST_TMDB_PAGE);
      result.items.reserve(page.results.size());

      for (const auto& mediaItem : page.results)
      {
         if (mediaItem.id == INVALID_TMDB_ID)
            continue;

         if (mediaItem.mediaType && *mediaItem.mediaType != TMDB_MEDIA_TYPE_MOVIE)
            continue;

         result.items.push_back(mediaItem.id);
      }

      return result;
   }
} // namespace loomis
