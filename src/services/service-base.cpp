#include "service-base.h"

#include "logger/logger.h"
#include "logger/log-utils.h"

#include <chrono>
#include <format>
#include <ranges>

namespace loomis
{
   ServiceBase::ServiceBase(std::string_view name,
                            std::string_view ansiiColor,
                            std::shared_ptr<ApiManager> apiManager,
                            const std::string& cronSchedule)
      : Base(name, ansiiColor, std::nullopt)
      , apiManager_(apiManager)
   {
      task_.service = true;
      task_.name = utils::GetAnsiText(name, ansiiColor);
      task_.cronExpression = cronSchedule;
      task_.func = [this]() { this->Run(); };
   }

   const Task& ServiceBase::GetTask() const
   {
      return task_;
   }

   const std::shared_ptr<ApiManager> ServiceBase::GetApiManager() const
   {
      return apiManager_;
   }
}