#pragma once

#include "api/api-manager.h"
#include "services/service-base.h"

#include <memory>
#include <string>

namespace loomis
{
   class TestService : public ServiceBase
   {
   public:
      TestService(std::string_view name,
                  std::string_view ansiiColor,
                  const std::string& cronExpression,
                  std::shared_ptr<ApiManager> apiManager);
      virtual ~TestService() = default;

      void Run() override;
   };
}