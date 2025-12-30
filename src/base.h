#pragma once

#include <optional>
#include <string>

namespace loomis
{
   class Base
   {
   public:
      Base(std::string_view className, std::string_view ansiiCode, std::optional<std::string_view> classExtra);
      virtual ~Base() = default;

   protected:
      // Log functions used by the base items to insert service header into message
      void LogTrace(const std::string& msg);
      void LogInfo(const std::string& msg);
      void LogWarning(const std::string& msg);
      void LogError(const std::string& msg);

   private:
      std::string logHeader_;
   };
}