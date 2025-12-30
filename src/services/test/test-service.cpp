#include "test-service.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

#include <format>
#include <ranges>

namespace loomis
{
   TestService::TestService(std::string_view name,
                            std::string_view ansiiColor,
                            const std::string& cronExpression,
                            std::shared_ptr<ApiManager> apiManager)
      : ServiceBase(name, ansiiColor, apiManager, cronExpression)
   {
   }

   void TestService::Run()
   {
      std::this_thread::sleep_for(std::chrono::seconds(2));
   }
}