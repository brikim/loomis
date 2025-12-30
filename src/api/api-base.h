#pragma once

#include "base.h"
#include "config-reader/config-reader-types.h"

#include <string>

namespace loomis
{
   class ApiBase : public Base
   {
   public:
      ApiBase(std::string_view name, const ServerConnectionConfig& serverConnection, std::string_view className, std::string_view ansiiCode);
      virtual ~ApiBase() = default;

      [[nodiscard]] const std::string& GetName() const;
      [[nodiscard]] const std::string& GetUrl() const;
      [[nodiscard]] const std::string& GetApiKey() const;

      [[nodiscard]] virtual bool GetValid() = 0;
      [[nodiscard]] virtual std::optional<std::string> GetServerReportedName() = 0;
      [[nodiscard]] virtual std::optional<std::string> GetLibraryId(std::string_view libraryName) = 0;
      virtual void SetLibraryScan(std::string_view libraryId) = 0;

   private:
      std::string name_;
      std::string url_;
      std::string apiKey_;
   };
}