#pragma once

#include "types.h"

#include <croncpp/croncpp.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace loomis
{
   struct CronTask
   {
      std::string name;
      cron::cronexpr cron;
      std::function<void(void)> func;
      std::chrono::system_clock::time_point nextRun;
   };

   class CronScheduler
   {
   public:
      CronScheduler() = default;
      virtual ~CronScheduler() = default;

      // Add a task to the scheduler
      void Add(const Task& task);

      // Start the scheduleres. Returns success.
      bool Start();

      // Shutdown the scheduler thread
      void Shutdown();

   private:
      void Work(std::stop_token stopToken);

      std::vector<CronTask> cronTasks_;

      std::atomic_bool shutdown_{false};

      std::mutex cvLock_;
      std::condition_variable cv_;
      std::unique_ptr<std::jthread> runThread_;
   };
}