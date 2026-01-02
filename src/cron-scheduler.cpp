#include "cron-scheduler.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

#include <format>

namespace loomis
{
   void CronScheduler::Add(const Task& task)
   {
      if (runThread_)
      {
         Logger::Instance().Error(std::format("Cron Scheduler: Attempted to add task {} after start", task.name));
         return;
      }

      try
      {
         auto& cronTask = cronTasks_.emplace_back();
         cronTask.name = task.name;
         cronTask.cron = cron::make_cron(task.cronExpression);
         cronTask.func = task.func;
         // Initialize the first run time immediately
         cronTask.nextRun = cron::cron_next(cronTask.cron, std::chrono::system_clock::now());
      }
      catch (const cron::bad_cronexpr& ex)
      {
         Logger::Instance().Error(std::format("Cron Scheduler: {} bad CRON {} - {}",
                                              task.name, task.cronExpression, ex.what()));
      }
   }

   void CronScheduler::Work(std::stop_token stopToken)
   {
      while (!stopToken.stop_requested())
      {
         auto currentTime = std::chrono::system_clock::now();
         auto nextWaitPoint = currentTime + std::chrono::years(1);

         // 1. Find the earliest nextRun
         for (const auto& task : cronTasks_)
         {
            if (task.nextRun < nextWaitPoint) nextWaitPoint = task.nextRun;
         }

         std::unique_lock<std::mutex> lock(cvLock_);

         cv_.wait_until(lock, stopToken, nextWaitPoint, [&] {
            return stopToken.stop_requested();
         });

         if (stopToken.stop_requested()) break;

         currentTime = std::chrono::system_clock::now();
         for (auto& task : cronTasks_)
         {
            if (task.nextRun <= currentTime)
            {
               Logger::Instance().Trace(std::format("Cron Scheduler: Executing {}", task.name));
               try
               {
                  task.func();
               }
               catch (const std::exception& e)
               {
                  Logger::Instance().Error(std::format("Task {} failed: {}", task.name, e.what()));
               }
               // Update nextRun for next time
               task.nextRun = cron::cron_next(task.cron, std::chrono::system_clock::now());
            }
         }
      }
      Logger::Instance().Info("Cron Scheduler: Work thread shutting down");
   }

   bool CronScheduler::Start()
   {
      if (cronTasks_.empty()) return false;

      // jthread starts immediately and manages its own lifetime
      runThread_ = std::make_unique<std::jthread>([this](std::stop_token st) { Work(st); });

      for (const auto& task : cronTasks_)
      {
         Logger::Instance().Info(std::format("{}: Enabled - Schedule: {}",
                                             task.name, cron::to_cronstr(task.cron)));
      }
      return true;
   }

   void CronScheduler::Shutdown()
   {
      if (runThread_)
      {
         runThread_->request_stop();

         cv_.notify_all();

         if (runThread_->joinable())
         {
            runThread_->join();
         }
         runThread_.reset();
      }
   }
}