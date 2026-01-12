#pragma once

#include "types.h"

#include <chrono>
#include <condition_variable>
#include <croncpp.h>
#include <mutex>
#include <thread>
#include <vector>

namespace loomis
{
   struct CronTask
   {
      Task task;
      cron::cronexpr cron;
      sys_clk::time_point nextRun;
   };

   class CronScheduler
   {
   public:
      CronScheduler() = default;
      ~CronScheduler();

      // Standardize: Add tasks before starting
      void Add(const Task& task);

      bool Start();
      void Shutdown();

   private:
      sys_clk::time_point GetNextRunTime() const;
      void RunTasks();
      void Work(std::stop_token stopToken);

      std::vector<CronTask> cronTasks_;

      std::mutex cvLock_;
      std::condition_variable_any cv_;

      std::unique_ptr<std::jthread> runThread_;
   };
}