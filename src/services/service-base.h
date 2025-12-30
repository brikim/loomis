#pragma once

#include "api/api-manager.h"
#include "base.h"
#include "types.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace loomis
{
   class ServiceBase : public Base
   {
   public:
      ServiceBase(std::string_view name,
                  std::string_view ansiiColor,
                  std::shared_ptr<ApiManager> apiManager,
                  const std::string& cronSchedule);
      virtual ~ServiceBase() = default;

      [[nodiscard]] const Task& GetTask() const;

   protected:
      [[nodiscard]] ApiManager* GetApiManager() const;

      // Function will be called at the returned cron schedule
      virtual void Run() = 0;

   private:
      Task task_;
      std::shared_ptr<ApiManager> apiManager_;
   };
}