#pragma once

#include "api/api-emby.h"
#include "api/api-jellystat.h"
#include "api/api-manager.h"
#include "api/api-tautulli-types.h"
#include "config-reader/config-reader-types.h"
#include "types.h"

#include <functional>

namespace loomis
{
   class EmbyUser
   {
   public:
      EmbyUser(const ServerUser& config,
               ApiManager* apiManager,
               const std::function<void(LogType, const std::string&)>& logFunc);
      virtual ~EmbyUser() = default;

      [[nodiscard]] bool GetValid() const;

      void Update();

      void SyncStateWithPlex(const TautulliHistoryItem* item, std::string_view path, std::string& target);

   private:
      bool SyncWatchedState(const TautulliHistoryItem* item, std::string_view path);
      bool SyncPlayState(const TautulliHistoryItem* item, std::string_view path);

      bool valid_{false};
      std::function<void(LogType, const std::string&)> logFunc_;
      ServerUser config_;

      EmbyApi* embyApi_{nullptr};
      JellystatApi* jellystatApi_{nullptr};
   };
}