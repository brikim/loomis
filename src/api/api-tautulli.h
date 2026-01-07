#pragma once

#include "api/api-base.h"
#include "api/api-tautulli-types.h"
#include "config-reader/config-reader-types.h"

#include <httplib.h>

#include <cstdint>
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

      [[nodiscard]] std::optional<std::vector<Task>> GetTaskList() override;

      // Returns true if the server is reachable and the API key is valid
      [[nodiscard]] bool GetValid() override;
      [[nodiscard]] std::optional<std::string> GetServerReportedName() override;

      [[nodiscard]] std::optional<TautulliUserInfo> GetUserInfo(std::string_view name);

      [[nodiscard]] std::optional<TautulliHistoryItems> GetWatchHistoryForUser(std::string_view user, std::string_view dateForHistory);

   private:
      std::string BuildApiPath(std::string_view cmd);

      // Server should be responding before making this call
      uint32_t GetWatchedPercent();

      bool ReadMonitoringData();
      void RunSettingsUpdate();

      httplib::Client client_;
      httplib::Headers headers_;

      std::optional<uint32_t> watchedPercent_;
   };
}