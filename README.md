# Loomis
Loomis is a collection of services to assist in your Plex and Emby media management.

1. TMDB Sync
* Creates and updates Emby playlists from TMDB movie lists.

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
        "enabled": true,
        "url": "http://0.0.0.0:0",
        "key": "apprise",
        "message_title": "Test loomis notification"
    },

    "watch_state_sync": {
        "enabled": true,
        "cron_run_rate": "0 0 */2 * * *",
        "users": [
            {"plex": [{"server": "Server1", "user_name": "User1", "can_sync": true}], "emby": [{"server": "Server1", "user_name": "User1"}, {"server": "Server2", "user_name": "User1"}]},
            {"plex": [{"server": "Server1", "user_name": "User2", "can_sync": false}], "emby": [{"server": "Server1", "user_name": "User2"}, {"server": "Server2", "user_name": "User2"}]}
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

#### TMDB Sync

TMDB playlists are movie-only; no media type needs to be configured. TV and mixed media types, along with TV-only playlist types, are not supported. Each playlist's `emby` list selects the Emby servers to update. Loomis resolves fetched TMDB movie IDs against each server's library, then creates or updates an Emby playlist with the configured name and TMDB ordering. Movies not present in an Emby library are skipped. A dry run performs lookups and reports planned playlist changes without modifying playlists.
