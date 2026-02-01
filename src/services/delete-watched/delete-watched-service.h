#pragma once

#include "config-reader/config-reader-types.h"
#include "services/delete-watched/delete-watched-library.h"
#include "services/service-base.h"

#include <warp/api/api-manager.h>

#include <memory>
#include <vector>

namespace loomis
{
   class DeleteWatchedService : public ServiceBase
   {
   public:
      DeleteWatchedService(const DeleteWatchedConfig& config,
                           std::shared_ptr<warp::ApiManager> apiManager);
      ~DeleteWatchedService() = default;

      void Run() override;

   private:
      void Init(const DeleteWatchedConfig& config);

      DeleteWatchedConfig config_;

      std::vector<std::unique_ptr<DeleteWatchedLibrary>> libraries_;
   };

} // namespace Loomis