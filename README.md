# sm64-san-andreas

![image](screenshot.png)

An ASI plugin for GTA San Andreas, which makes use of [libsm64](https://github.com/libsm64/libsm64) to allow you to play as Mario from Super Mario 64.

This plugin requires the following in order to work:
* [Silent's ASI Loader](https://www.gtagarage.com/mods/show.php?id=21709)
* Super Mario 64 US ROM, with the filename `sm64.us.z64`, located in the GTA San Andreas folder alongside `gta_sa.exe`
* IF YOU HAVE THE Delisted **Steam** version (NOT Definitive Edition): Use the [San Andreas Downgrader](https://gtaforums.com/topic/927016-san-andreas-downgrader/) to bring the game back to v1.0

It's also recommended to install the [SilentPatchSA](https://gtaforums.com/topic/669045-silentpatch) mod.
It's not necessary in order for this plugin to work, but it fixes an issue with Mario's shading appearing to be flat while outdoors.

Once you are ingame, press the M key to switch between Mario and CJ.

This is still under development!

## How to download
### From releases page
* Go to the [Releases](https://github.com/headshot2017/sm64-san-andreas/releases) page
* Go to "Assets" and download "sm64-san-andreas-release.zip"
* Extract the zip's contents to your GTA SA "scripts" folder.

### Nightly builds
**This requires you to be logged in to your GitHub account.**
* Go to the [Actions](https://github.com/headshot2017/sm64-san-andreas/actions) page
* Click the first item that appears in the "workflow runs" list
* Scroll down to "Artifacts" and click "sm64-san-andreas-release"
* Extract the zip's contents to your GTA SA "scripts" folder.

## Compiling
See the [Compiling the plugin](https://github.com/headshot2017/sm64-san-andreas/wiki/Compiling-the-plugin) wiki page for a complete tutorial for setting up CodeBlocks, mingw-w64, plugin-sdk, and this plugin.

NOTE: The build targets for both projects must match! e.g. if libsm64 is built with the Release target, sm64-san-andreas must also be built with the GTASA Release target.

## Known issues
* Using this with the SkyGFX mod makes Mario extremely bright, and the textures in his face (eyes, moustache, hat logo...) don't render correctly.
  * Disable "dualPass" in all the skygfx .ini files (change 1 to 0) to fix this issue.
