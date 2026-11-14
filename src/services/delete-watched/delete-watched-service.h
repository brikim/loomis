#pragma once

#include "services/delete-watched/delete-watched-library.h"
#include "services/service-base.h"

#include <warp/api/api-manager.h>
#include <warp/api/api-tracearr.h>

#include "config-reader/config-reader-types.h"

#include <memory>
#include <vector>

namespace loomis
{
   class DeleteWatchedService : public ServiceBase
   {
   public:
      DeleteWatchedService(const DeleteWatchedConfig& config,
                           std::shared_ptr<warp::ApiManager> apiManager);
      ~DeleteWatchedService() override = default;

      void Run() override;

   private:
      void Init(const DeleteWatchedConfig& config);

      std::vector<std::string> validUsers_;
      warp::TracearrApi* tracearrApi_{nullptr};
      std::vector<std::unique_ptr<DeleteWatchedLibrary>> libraries_;
   };

} // namespace loomis
