#include "config-reader/config-reader.h"
#include "service-manager.h"
#include "version.h"

#include <warp/log.h>

#include <csignal>
#include <memory>

std::unique_ptr<loomis::ServiceManager> SERVICE_MANAGER;

void signal_handler(int signal_num)
{
   if ((signal_num == SIGINT || signal_num == SIGTERM)
       && SERVICE_MANAGER)
   {
      SERVICE_MANAGER->ProcessShutdown();
   }
}

void init_logging(const std::shared_ptr<loomis::ConfigReader>& configReader)
{
   if (const auto* logPath = std::getenv("LOG_PATH");
       logPath)
   {
      warp::log::InitFileLogging(logPath, "loomis.log");
   }

   // Initialize Apprise logging if configured
   const auto& appriseConfig = configReader->GetAppriseLogging();
   warp::AppriseLoggingConfig loomlogAppriseConfig;
   loomlogAppriseConfig.enabled = appriseConfig.enabled;
   loomlogAppriseConfig.url = appriseConfig.url;
   loomlogAppriseConfig.key = appriseConfig.key;
   loomlogAppriseConfig.message_title = appriseConfig.message_title;
   warp::log::InitApprise(loomlogAppriseConfig);
}

int main()
{
   // Initialize the config reader. This class will use the logger so initialize it after.
   auto configReader{std::make_shared<loomis::ConfigReader>()};

   // Initialize the logging system
   init_logging(configReader);

   // Check for config file validity. If not valid exit logging the error.
   // This file is required for this application to run
   if (configReader->IsConfigValid() == false)
   {
      warp::log::Critical("Config file not valid shutting down");
      return 1;
   }

   warp::log::Info("Loomis {} Starting", loomis::LOOMIS_VERSION);

   SERVICE_MANAGER = std::make_unique<loomis::ServiceManager>(configReader);

   // Register to handle the required signals
   std::signal(SIGINT, signal_handler);
   std::signal(SIGTERM, signal_handler);

   SERVICE_MANAGER->Run();

   return 0;
}