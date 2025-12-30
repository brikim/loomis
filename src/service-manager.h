#pragma once

#include "api/api-manager.h"
#include "config-reader/config-reader.h"
#include "config-reader/config-reader-types.h"
#include "cron-scheduler.h"
#include "services/service-base.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace loomis
{
   class ServiceManager
   {
   public:
      ServiceManager(std::shared_ptr<ConfigReader> configReader);
      virtual ~ServiceManager() = default;

      void Run();

      void ProcessShutdown();

   private:
      void CreateServices();

      std::shared_ptr<ConfigReader> configReader_;
      std::shared_ptr<ApiManager> apiManager_;
      CronScheduler cronScheduler_;

      std::atomic_bool shutdownService_{false};
      std::mutex runCvLock_;
      std::condition_variable runCv_;

      std::vector<std::unique_ptr<ServiceBase>> services_;
   };
}