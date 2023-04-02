# sm64-san-andreas

![it's a me](screenshot.png)

An ASI plugin for GTA San Andreas, which makes use of [libsm64](https://github.com/libsm64/libsm64) to allow you to play as Mario from Super Mario 64.

This plugin requires the following in order to work:
* [Silent's ASI Loader](https://www.gtagarage.com/mods/show.php?id=21709)
* Super Mario 64 US ROM, with the filename `sm64.us.z64`, located in the GTA San Andreas folder alongside `gta_sa.exe`

It's also recommended to install the [SilentPatchSA](https://gtaforums.com/topic/669045-silentpatch) mod.
It's not necessary in order for this plugin to work, but it fixes an issue with Mario's shading appearing to be flat while outdoors.

This is still under development!

## Compiling
See the [Compiling the plugin](https://github.com/headshot2017/sm64-san-andreas/wiki/Compiling-the-plugin) wiki page for a complete tutorial for setting up CodeBlocks, mingw-w64, plugin-sdk, and this plugin.

NOTE: The build targets for both projects must match! e.g. if libsm64 is built with the Release target, sm64-san-andreas must also be built with the GTASA Release target.
