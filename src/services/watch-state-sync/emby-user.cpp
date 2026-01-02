#include "emby-user.h"

#include "logger/log-utils.h"

namespace loomis
{
   EmbyUser::EmbyUser(const ServerUser& config,
                      ApiManager* apiManager,
                      const std::function<void(LogType, const std::string&)>& logFunc)
      : logFunc_(logFunc)
      , config_(config)
   {
      // Do some quick checking on the users and make sure the api in the config exists.
      // Don't want to check if the user is valid on the api yet since it might be offline.
      // This will be checked every run frame.
      embyApi_ = apiManager->GetEmbyApi(config_.server);
      jellystatApi_ = apiManager->GetJellystatApi(config_.server);
      if (embyApi_ && jellystatApi_)
      {
         // Will get users from emby. Do a small pre-check and warn the system.
         if (embyApi_->GetValid() && !embyApi_->GetUserExists(config_.user))
         {
            logFunc_(LogType::WARN, std::format("{} not found on {}. Is user name correct?",
                                                utils::GetTag("user", config_.user),
                                                utils::GetServerName(utils::GetFormattedEmby(), config_.server)));
         }

         valid_ = true;
      }
      else
      {
         if (!embyApi_)
         {
            logFunc_(LogType::WARN, std::format("{} api not found for {}",
                                                utils::GetServerName(utils::GetFormattedEmby(), config_.server),
                                                utils::GetTag("user", config_.user)));
         }

         if (!jellystatApi_)
         {
            logFunc_(LogType::WARN, std::format("{} tracker api not found for {}. Required for this service.",
                                                utils::GetServerName(utils::GetFormattedJellystat(), config_.server),
                                                utils::GetTag("user", config_.user)));
         }
      }
   }

   bool EmbyUser::GetValid() const
   {
      return valid_;
   }

   void EmbyUser::Update()
   {

   }

   bool EmbyUser::SyncWatchedState(const TautulliHistoryItem* item)
   {
      return false;
   }

   bool EmbyUser::SyncPlayState(const TautulliHistoryItem* item)
   {
      return false;
   }

   void EmbyUser::SyncStateWithPlex(const TautulliHistoryItem* item, std::string_view path, std::string& target)
   {
      if (item->watched ? SyncWatchedState(item) : SyncPlayState(item))
      {
         target = utils::BuildTargetString(target, utils::GetFormattedEmby(), config_.server);
      }
   }
}