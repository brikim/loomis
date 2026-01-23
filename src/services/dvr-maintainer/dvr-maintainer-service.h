#pragma once

#include "config-reader/config-reader-types.h"
#include "services/dvr-maintainer/dvr-library.h"
#include "services/service-base.h"

#include <memory>
#include <vector>

class ApiManager;

namespace loomis
{
   class DvrMaintainerService : public ServiceBase
   {
   public:
      DvrMaintainerService(const DvrMaintainerConfig& config,
                           std::shared_ptr<ApiManager> apiManager);
      virtual ~DvrMaintainerService() = default;

      void Run() override;

   private:
      void Init(const DvrMaintainerConfig& config);

      DvrMaintainerConfig config_;

      std::vector<std::unique_ptr<DvrLibrary>> libraries_;
   };

} // namespace Loomis