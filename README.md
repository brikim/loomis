# Loomis
Loomis is a collection of services to assist in your Plex and Emby media management.

1. Playlist Sync
* Allows the syncing of Plex Collections to Emby Playlists

Each service can be configured independently. Each service allows for a cron rate on when to run.

## Installing Loomis
Loomis offers a pre-compiled [docker image](https://hub.docker.com/repository/docker/brikim/loomis/general)

### Usage
Use docker compose to run Loomis

### compose.yml
```yaml
---
services:
  loomis:
    image: brikim/loomis:latest
    container_name: loomis
    security_opt:
      - no-new-privileges:true
    environment:
      - TZ=America/Chicago
    volumes:
      - /docker/loomis/config:/config
      - /docker/loomis/logs:/logs
      - /pathToMedia:/media
    restart: unless-stopped
```
> [!NOTE]
> 📝 /media folder can not be read only for all services to function correctly

### Environment Variables
| Env | Function |
| :------- | :------------------------ |
| TZ       | specify a timezone to use |

### Volume Mappings
| Volume | Function |
| :------- | :------------------------ |
| /config  | Path to a folder containing config.yml used to setup Loomis |
| /logs    | Path to a folder to store Loomis log files |
| /media   | Path to your media files. Used by services to monitor your media files |

### Configuration File
A configuration file is required to use Loomis. Create a config.conf file in the volume mapped to /config

#### config.conf
```yaml
{
    "plex": {
        "servers": [
            {
                "server_name": "Server1",
                "url": "http://0.0.0.0:32400",
                "api_key": "",
                "tracker_url": "http://0.0.0.0:0",
                "tracker_api_key": "",
                "media_path": "/media/"
            },
            {
                "server_name": "Server2",
                "media_path": "/media/",
                "url": "http://0.0.0.0:32401",
                "api_key": "",
                "tracker_url": "http://0.0.0.0:0",
                "tracker_api_key": ""
            }
        ]
    },

    "emby": {
        "servers": [
            {
                "server_name": "Server1",
                "url": "http://0.0.0.0:8096",
                "api_key": "",
                "tracker_url": "http://0.0.0.0:0",
                "tracker_api_key": "",
                "media_path": "/media/"
            },
            {
                "server_name": "Server2",
                "url": "http://0.0.0.0:8097",
                "api_key": "",
                "tracker_url": "http://0.0.0.0:0",
                "tracker_api_key": "",
                "media_path": "/media/"
            }
        ],
    },

    "apprise_logging": {
        "enabled": "True",
        "url": "http://0.0.0.0:0",
        "key": "apprise",
        "message_title": "Test remote scan notification"
    },

    "playlist_sync": {
        "enabled": "True",
        "cron_comment": "Non-Standard cron expression. First digit is seconds so leave this as 0 if this accuracy is not needed and fill in the rest with standard cron expression",
        "cron": "0 0 */2 * * *",
        "time_for_emby_to_update_seconds": 1,
        "time_between_syncs_seconds": 1,
        "plex_collection_sync": [
            {"server": "Server1", "library": "Server1_LibraryName", "collection_name": "plexCollectionName", "target_emby_servers": [{"server": "Server1"}, {"server": "Server2"}]}
        ]
    }
}
```

#### Option Descriptions
You only have to define the variables for servers in your system. For plex only define plex_url and plex_api_key in your file. The emby and jellyfin variables are not required.
| Media Server | Function |
| :----------- | :------------------------ |
| plex                 | A list of plex servers used |
| emby                 | A list of emby servers used |

#### Apprise Logging
Not required unless wanting to send Warnings or Errors to Apprise
| Apprise | Function |
| :--------------- | :------------------------ |
| enabled          | Enable the function with 'True' |
| url              | Url including port to your apprise server |
| key              | Apprise key to be used to send notifications |
| message_title    | Title to put in the title bar of the message |

#### Playlist Sync
Playlist Sync will sync Plex Collections to Emby Playlists with the same name. This will run at the scheduled rate and update the Emby playlist to match the Plex collection.

| playlist_sync | Function |
| :--------------- | :------------------------ |
| enabled                           | Enable the sync watch service |
| cron                              | Rate at which to run this service. Non-Standard cron expression. First digit is seconds so leave this as 0 if this accuracy is not needed and fill in the rest with standard cron expression |
| time_for_emby_to_update_seconds   | How many seconds to give the Emby server to update |
| time_between_syncs_seconds        | How many seconds to give the Emby server between sync updates |

1 to many plex collections can be synced
| plex_collection_sync | Function |
| :--------------- | :------------------------ |
| server                | The name of the plex server for this collection. Must be in the plex server list. |
| library               | The name of the plex library that contains this collection. |
| collection_name       | The name of the collection to sync. |
| target_emby_servers   | A list of emby servers to sync the plex collection. |
