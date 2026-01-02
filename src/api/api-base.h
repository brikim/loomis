#pragma once

#include "base.h"
#include "config-reader/config-reader-types.h"
#include "types.h"

#include <httplib/httplib.h>
#include <json/json.hpp>

#include <optional>
#include <string>

namespace loomis
{
   class ApiBase : public Base
   {
   public:
      ApiBase(std::string_view name, const ServerConnectionConfig& serverConnection, std::string_view className, std::string_view ansiiCode);
      virtual ~ApiBase() = default;

      // Api tasks are optional. Api's can override to perform a task
      [[nodiscard]] virtual std::optional<Task> GetTask();

      [[nodiscard]] const std::string& GetName() const;
      [[nodiscard]] const std::string& GetUrl() const;
      [[nodiscard]] const std::string& GetApiKey() const;

      [[nodiscard]] virtual bool GetValid() = 0;
      [[nodiscard]] virtual std::optional<std::string> GetServerReportedName() = 0;

   protected:
      void AddApiParam(std::string& url, const std::list<std::pair<std::string_view, std::string_view>>& params) const;

      // Encode the source string to percent encoding
      std::string GetPercentEncoded(std::string_view src) const;

      // Returns if the http request was successful and outputs to the log if not successful
      bool IsHttpSuccess(std::string_view name, const httplib::Result& result);

   private:
      std::string name_;
      std::string url_;
      std::string apiKey_;
   };
}