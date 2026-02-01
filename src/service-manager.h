#pragma once

#include "config-reader/config-reader.h"
#include "config-reader/config-reader-types.h"
#include "services/service-base.h"

#include <warp/api/api-manager.h>
#include <warp/scheduler/cron-scheduler.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stop_token>
#include <vector>

namespace loomis
{
   class ServiceManager
   {
   public:
      explicit ServiceManager(std::shared_ptr<ConfigReader> configReader);
      ~ServiceManager() = default;

      // Prevents the main thread from exiting
      void Run();

      // Triggers the shutdown sequence
      void ProcessShutdown();

   private:
      void CreateServices();

      // Re-ordered slightly for safety
      std::shared_ptr<ConfigReader> configReader_;
      std::shared_ptr<warp::ApiManager> apiManager_;

      // Services often depend on APIs, so declare services after APIs
      std::vector<std::unique_ptr<ServiceBase>> services_;

      // Scheduler is last to ensure it's first to stop during destruction
      warp::CronScheduler cronScheduler_;

      std::stop_source stopSource_;
   };
}