#pragma once

#include "types.h"
#include <chrono>
#include <condition_variable>
#include <croncpp/croncpp.h>
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
      std::function<void()> func;
      std::chrono::system_clock::time_point nextRun;
   };

   class CronScheduler
   {
   public:
      CronScheduler() = default;
      // Ensure the thread is stopped before the object is destroyed
      ~CronScheduler()
      {
         Shutdown();
      }

      // Standardize: Add tasks before starting
      void Add(const Task& task);

      bool Start();
      void Shutdown();

   private:
      // The worker thread logic
      void Work(std::stop_token stopToken);

      std::vector<CronTask> cronTasks_;

      // Mutex and CV_ANY are required for the C++20 stop_token pattern
      std::mutex cvLock_;
      std::condition_variable_any cv_;

      // jthread manages its own stop_state and joins on destruction
      std::unique_ptr<std::jthread> runThread_;
   };
}