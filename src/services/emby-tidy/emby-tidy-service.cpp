#include "emby-tidy-service.h"

#include "services/service-types.h"

#include <warp/log/log-utils.h>

namespace loomis
{
   namespace
   {
      constexpr std::string_view SERVICE_NAME("Emby Tidy");
   }

   EmbyTidyService::EmbyTidyService(const EmbyTidyConfig& config,
                                    std::shared_ptr<warp::ApiManager> apiManager)
      : ServiceBase(SERVICE_NAME, ANSI_CODE_SERVICE_EMBY_TIDY, apiManager, config.cron)
      , dryRun_(config.dryRun)
   {
      Init(config);
   }

   void EmbyTidyService::Init(const EmbyTidyConfig& config)
   {
      if (dryRun_) LogInfo("[DRY RUN] Enabled - No emby items will be modified");

      for (const auto& server : config.servers)
      {
         auto api = GetApiManager()->GetEmbyApi(server.name);
         if (!api)
         {
            LogWarning("No {} found with {}",
                       warp::GetFormattedEmby(),
                       warp::GetTag("name", server.name));
            continue;
         }

         EmbyTidyService::ServerConfig serverConfig;
         serverConfig.api = api;
         serverConfig.cleanUpNonLocalBackdrops = server.cleanUpNonLocalBackdrops;
         serverConfig.pathPrefix = server.localBackdropContainerStartPath.lexically_normal();
         serverConfig.libraries.reserve(server.libraries.size());

         if (api->GetValid())
         {
            for (const auto& library : server.libraries)
            {
               if (auto libId = api->GetLibraryId(library.name);
                   libId)
               {
                  serverConfig.libraries.emplace_back(library.name);
               }
               else
               {
                  LogWarning("{} does not have {} ... Skipping",
                             api->GetPrettyName(),
                             warp::GetTag("library", library.name));
               }
            }
         }
         else
         {
            std::ranges::for_each(server.libraries, [&serverConfig](const auto& library) {
               serverConfig.libraries.emplace_back(library.name);
            });
         }

         if (serverConfig.libraries.size() > 0)
         {
            servers_.emplace_back(std::move(serverConfig));
         }
      }
   }

   void EmbyTidyService::CleanNonLocalBackdrops(const ServerConfig& config)
   {
      auto items = config.api->GetAllItemsBackdrop();
      for (const auto& item : items)
      {
         if (item.backdropImageIds.size() <= 1) continue;

         auto backdrops = config.api->GetBackdrops(item.id);
         if (backdrops.size() <= 1) continue;

         bool containsLocalBackdrop = false;
         std::vector<int32_t> removeBackdropIndices;
         for (const auto& backdrop : backdrops)
         {
            auto current = backdrop.path.lexically_normal();

            auto [it_prefix, it_full] = std::mismatch(
                config.pathPrefix.begin(), config.pathPrefix.end(),
                current.begin(), current.end()
            );

            bool isLocal = (it_prefix == config.pathPrefix.end());
            if (isLocal)
            {
               containsLocalBackdrop = true;
            }
            else
            {
               removeBackdropIndices.emplace_back(backdrop.index);
            }
         }

         // If this item doesn't contain a local backdrop skip
         if (!containsLocalBackdrop || removeBackdropIndices.empty()) continue;

         // Sort the list so the largest indices are first. This guarentees the
         // images are deleted and no indices changes from emby
         std::ranges::sort(removeBackdropIndices, std::greater<int32_t>());

         for (auto removeIndex : removeBackdropIndices)
         {
            if (!dryRun_)
            {
               if (config.api->RemoveBackdropImage(item.id, removeIndex))
               {
                  LogInfo("Removed backdrop from {} {}",
                          warp::GetTag("name", item.name),
                          warp::GetTag("index", removeIndex));
               }
            }
            else
            {
               LogInfo("[DRY RUN] Would remove backdrop from {} {}",
                       warp::GetTag("name", item.name),
                       warp::GetTag("index", removeIndex));
            }
         }
      }
   }

   void EmbyTidyService::Run()
   {
      for (const auto& server : servers_)
      {
         if (server.cleanUpNonLocalBackdrops)
         {
            CleanNonLocalBackdrops(server);
         }
      }
   }
}