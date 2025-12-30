#pragma once

#include "api/api-base.h"
#include "config-reader/config-reader-types.h"

#include <httplib/httplib.h>

#include <list>
#include <optional>
#include <string>

namespace loomis
{
   class TautulliApi : public ApiBase
   {
   public:
      TautulliApi(const ServerConfig& serverConfig);
      virtual ~TautulliApi() = default;

      // Returns true if the server is reachable and the API key is valid
      [[nodiscard]] bool GetValid() override;
      [[nodiscard]] std::optional<std::string> GetServerReportedName() override;
      [[nodiscard]] std::optional<std::string> GetLibraryId(std::string_view libraryName) override
      {
         return std::nullopt;
      };

   private:
      std::string BuildApiPath(std::string_view path);

      httplib::Client client_;
      httplib::Headers headers_;
   };
}