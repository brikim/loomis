#pragma once

#include <warp/api/api-manager.h>
#include <warp/base.h>
#include <warp/types.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace loomis
{
   class ServiceBase : public warp::Base
   {
   public:
      ServiceBase(std::string_view name,
                  std::string_view ansiiColor,
                  std::shared_ptr<warp::ApiManager> apiManager,
                  const std::string& cronSchedule);
      virtual ~ServiceBase() = default;

      [[nodiscard]] const warp::Task& GetTask() const;

   protected:
      [[nodiscard]] const std::shared_ptr<warp::ApiManager> GetApiManager() const;

      // Function will be called at the returned cron schedule
      virtual void Run() = 0;

   private:
      warp::Task task_;
      std::shared_ptr<warp::ApiManager> apiManager_;
   };
}