# HoloCure Coin Tools Mod
A HoloCure mod intended to assist with optimizing coin runs which adds features to track various stats throughout your run and log runs automatically at the end.

# Installation

Regarding the YYToolkit Launcher:

It's a launcher used to inject .dlls, so most anti-virus will be quick to flag it with Trojan-like behavior because of a similar use-case. The launcher is entirely open source (as is YYToolkit itself, the backbone of the project), so you're more than welcome to build everything yourself from the source: https://github.com/Archie-osu/YYToolkit/

- Download the .dll file from the latest release of my [Coin Tools mod](https://github.com/mashirochan/HoloCure-CoinTools/releases/latest)
- Download `Launcher.exe` from the [latest release of YYToolkit](https://github.com/Archie-osu/YYToolkit/releases/latest)
  - Place the `Launcher.exe` file anywhere you want for convenient access
- Open the folder your `HoloCure.exe` is in
  - Back up your game and save data while you're here!
  - Delete, rename, or move `steam_api64.dll` and `Steamworks_x64.dll` if you're on Steam
- Run the `Launcher.exe`
  - Click "Select" next to the Runner field
    - Select your `HoloCure.exe` (wherever it is)
  - Click "Open plugin folder" near the bottom right
    - This should create and open the `autoexec` folder wherever your `HoloCure.exe` is
    - Move or copy your `coin-tools-vX.X.X.dll` file into this folder
  - Click "Start process"
    - Hope with all your might that it works!

Not much testing at all has gone into this, so I'm really sorry if this doesn't work. Use at your own risk!

Feel free to join the [HoloCure Discord server](https://discord.gg/holocure) and look for the HoloCure Code Discussion thread in #holocure-general!

# Troubleshooting

Here are some common problems you could have that are preventing your mod from not working correctly:

### YYToolkit Launcher Hangs on "Waiting for game..."
![Waiting for game...](https://i.imgur.com/DxDjOGz.png)

The most likely scenario for this is that you did not delete, rename, or move the `steam_api64.dll` and `Steamworks_x64.dll` files in whatever directory the `HoloCure.exe` that you want to mod is in.

### Failed to install plugin: coin-tools.dll
![Failed to install plugin](https://i.imgur.com/fcg1WWe.png)

The most likely scenario for this is that you tried to click "Add plugin" before "Open plugin folder", so the YYToolkit launcher has not created an `autoexec` folder yet. To solve this, either click "Open plugin folder" to create an `autoexec` folder automatically, or create one manually in the same directory as your `HoloCure.exe` file.
