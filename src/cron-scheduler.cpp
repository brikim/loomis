#include "cron-scheduler.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

#include <format>

namespace loomis
{
   void CronScheduler::Add(const Task& task)
   {
      // Simple scheduler. All tasks must be added before starting the scheduler
      if (runThread_)
      {
         Logger::Instance().Error(std::format("Cron Scheduler: Attempted to add task {} after start", task.name));
         return;
      }

      try
      {
         auto cron{cron::make_cron(task.cronExpression)};

         auto& cronTask{cronTasks_.emplace_back()};
         cronTask.name = task.name;
         cronTask.cron = cron;
         cronTask.func = task.func;
      }
      catch (cron::bad_cronexpr const& ex)
      {
         Logger::Instance().Error(std::format("Cron Scheduler: {} attempted to schedule bad CRON {} {}", task.name, task.cronExpression, utils::GetTag("error", ex.what())));
      }
   }

   void CronScheduler::Work(std::stop_token stopToken)
   {
      while (!shutdown_.load() && !stopToken.stop_requested())
      {
         // Find all the next run times for all the tasks
         auto currentTime{std::chrono::system_clock::now()};
         auto nextTimePoint{currentTime + std::chrono::years(1)};
         for (auto& task : cronTasks_)
         {
            task.nextRun = cron::cron_next(task.cron, currentTime);
            if (task.nextRun < nextTimePoint)
            {
               nextTimePoint = task.nextRun;
            }
         }

         // Hold the thread until the next task runtime
         std::unique_lock<std::mutex> lock(cvLock_);
         if (!cv_.wait_until(lock, nextTimePoint, [this, &stopToken] { return shutdown_.load() || stopToken.stop_requested(); }))
         {
            // Loop through all the tasks and see if it is time to run.
            std::ranges::for_each(cronTasks_, [](auto& task) {
               if (auto currentTime{std::chrono::system_clock::now()};
                   task.nextRun <= currentTime)
               {
                  Logger::Instance().Trace(std::format("{}: Running Task", task.name));
                  task.func();
               }
            });
         }
      }

      Logger::Instance().Info("Shutting down work thread");
   }

   bool CronScheduler::Start()
   {
      // Simple scheduler all tasks must be added before start is called
      if (cronTasks_.empty())
      {
         return false;
      }

      // Create the thread to monitor active scans
      runThread_ = std::make_unique<std::jthread>([this](std::stop_token stopToken) {
         this->Work(stopToken);
      });

      // Log all the tasks that have started
      for (const auto& task : cronTasks_)
      {
         Logger::Instance().Info(std::format("{}: Enabled - Running on CRON schedule {}", task.name, cron::to_cronstr(task.cron)));
      }

      return true;
   }

   void CronScheduler::Shutdown()
   {
      {
         std::unique_lock<std::mutex> lock(cvLock_);
         shutdown_.store(true);
         cv_.notify_all();
      }

      if (runThread_)
      {
         runThread_->join();
      }
   }
}