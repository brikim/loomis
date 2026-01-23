#pragma once

#include "base.h"
#include "config-reader/config-reader-types.h"
#include "types.h"

#include <httplib.h>

#include <optional>
#include <string>
#include <vector>

namespace loomis
{
   using ApiParams = std::vector<std::pair<std::string_view, std::string_view>>;

   struct ApiBaseData
   {
      std::string_view name;
      std::string_view url;
      std::string_view apiKey;
      std::string_view className;
      std::string_view ansiiCode;
      std::string_view prettyName;
   };

   class ApiBase : public Base
   {
   public:
      ApiBase(const ApiBaseData& data);
      virtual ~ApiBase() = default;

      // Api tasks are optional. Api's can override to perform a task
      [[nodiscard]] virtual std::optional<std::vector<Task>> GetTaskList();

      [[nodiscard]] const std::string& GetName() const;
      [[nodiscard]] const std::string& GetPrettyName() const;
      [[nodiscard]] const std::string& GetUrl() const;
      [[nodiscard]] const std::string& GetApiKey() const;

      [[nodiscard]] virtual bool GetValid() = 0;
      [[nodiscard]] virtual std::optional<std::string> GetServerReportedName() = 0;

   protected:
      [[nodiscard]] virtual std::string_view GetApiBase() const = 0;
      [[nodiscard]] virtual std::string_view GetApiTokenName() const = 0;

      httplib::Client& GetClient();

      void AddApiParam(std::string& url, const ApiParams& params) const;
      [[nodiscard]] std::string BuildApiPath(std::string_view path) const;
      [[nodiscard]] std::string BuildApiParamsPath(std::string_view path, const ApiParams& params) const;

      // Returns if the http request was successful and outputs to the log if not successful
      bool IsHttpSuccess(std::string_view name, const httplib::Result& result, bool log = true);

   private:
      std::string name_;
      std::string prettyName_;
      std::string url_;
      std::string apiKey_;

      httplib::Client client_;
   };
}