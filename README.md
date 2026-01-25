# GtoMnK
is a simple tool that lets you use your Xbox controller to play games that don't have controller support.
It works by making your controller pretend to be a mouse and keyboard. All you need to do is inject or load the DLL into your game.

### Usage:
GtoMnK reads its configuration settings from the [`GtoMnK.ini`](https://github.com/SAM1430B/GtoMnK/blob/master/GtoMnK_DLL/INI/GtoMnK.ini) file and maps controller buttons using [key codes](https://github.com/SAM1430B/GtoMnK/blob/master/GtoMnK_DLL/Documents/GtoMnK_Keys.txt).
So you need to copy `GtoMnK32.dll`, `EasyHook32.dll`, `GtoMnK.ini` and one of the `Loader.x32` files to the game executable. And do same process for x64 version.

> [!WARNING]
> Games that have Anti-Cheat are more likely to detect the DLL as a cheat tool.

#### Profile version:
This version is intended for configuring and testing your setup. You can create configurations using the `GtoMnK_Config.html` file in the `Config` folder.
It also includes a `GtoMnK_Log.txt` file for diagnosing problems and provides advanced overlay settings.

#### Release version:
This version is for use after you have finalized your configuration. It is a streamlined version of the [Profile version](https://github.com/SAM1430B/GtoMnK#release-version), containing only essential overlay settings.

#### Overlay settings:
Press `Back` + `Dpad Down` to open the Overlay. You can change the Deadzones for the thumbsticks and change the sensitivity in real time.

Profile version has addition settings for tuning the sensititvity curve such as Curve_Slope and Curve_Exponent.

### Credits:

- Messenils the original auther of [screenshot-input](https://github.com/Messenils/screenshot-input).
- Ilyaki [ProtoInput](https://github.com/Ilyaki/ProtoInput).
- Nemirtingas [OpenXinput](https://github.com/Nemirtingas/OpenXinput).
- elishacloud [DirectX-Wrappers](https://github.com/elishacloud/DirectX-Wrappers).
- hauzer [dinput8_nojoy](https://github.com/hauzer/dinput8_nojoy)
