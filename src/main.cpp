#include "config-reader/config-reader.h"
#include "logger/logger.h"
#include "service-manager.h"
#include "version.h"

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

int main()
{
   // Initialize the logger
   loomis::Logger::Instance();

   // Initialize the config reader. This class will use the logger so initialize it after.
   auto configReader{std::make_shared<loomis::ConfigReader>()};
   loomis::Logger::Instance().InitApprise(configReader->GetAppriseLogging());

   // Check for config file validity. If not valid exit logging the error.
   // This file is required for this application to run
   if (configReader->IsConfigValid() == false)
   {
      loomis::Logger::Instance().Critical("Config file not valid shutting down");
      return 1;
   }

   loomis::Logger::Instance().Info("Loomis {} Starting", loomis::LOOMIS_VERSION);

   SERVICE_MANAGER = std::make_unique<loomis::ServiceManager>(configReader);

   // Register to handle the required signals
   std::signal(SIGINT, signal_handler);
   std::signal(SIGTERM, signal_handler);

   SERVICE_MANAGER->Run();

   return 0;
}