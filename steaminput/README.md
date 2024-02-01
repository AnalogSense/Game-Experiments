## Analog keyboard support for games using SteamInput

This is a proof-of-concept, currently developed and tested only against [Teardown](https://store.steampowered.com/app/1167630/Teardown/), but I am confident this approach will work for other games with only minor tweaks. Please open a Github issue if there's a game you'd like me to look at.

### Prerequisites

The Wooting Anlog SDK has to be installed. You can find the latest installers [here](https://github.com/WootingKb/wooting-analog-sdk/releases).

If you have a Razer Huntsman Analog keyboard, you will need to install [the respective SDK plugin](https://github.com/calamity-inc/universal-analog-plugin).

### Installation

1. Open the game folder.
2. Rename the `steam_api64.dll` to `steam_api64_og.dll`. If there is no `steam_api64.dll`, this game is not compatible.
3. Obtain the `steam_api64.dll` from this project and drop it into the game folder.

### Updating

Simply repeat installation step 3 as needed.

### Deinstallation

1. Open the game folder.
2. Delete the `steam_api64.dll`.
3. Rename the `steam_api64_og.dll` back to `steam_api64.dll`.

If you messed up anything, simply use "Verify Game Files" in Steam and it should also restore the game back to a healthy state.
