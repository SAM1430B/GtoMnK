# GtoMnK
is a simple tool that lets you use your Xbox controller to play games that don't have controller support.
It works by making your controller pretend to be a mouse and keyboard. All you need to do is inject or load the DLL into your game.

### Usage:
GtoMnK reads its configuration settings from the [`GtoMnK.ini`](https://github.com/SAM1430B/GtoMnK/blob/master/GtoMnK_DLL/INI/GtoMnK.ini) file and maps controller buttons using [key codes](https://github.com/SAM1430B/GtoMnK/blob/master/GtoMnK_DLL/Documents/GtoMnK_Keys.txt).

#### Profile version:
This version is intended for configuring and testing your setup. You can create configurations using the `GtoMnK_Config.html` file in the `Config` folder.
It also includes a `GtoMnK_Log.txt` file for diagnosing problems and provides advanced overlay settings.

#### Release version:
This version is for use after you have finalized your configuration. It is a streamlined version of the [Profile version](https://github.com/SAM1430B/GtoMnK#release-version), containing only essential overlay settings.

> [!WARNING]
> Games that have Anti-Cheat are more likely to detect the DLL as a cheat tool.

### Credits:

- Messenils the original auther of [screenshot-input](https://github.com/Messenils/screenshot-input).
- Ilyaki [ProtoInput](https://github.com/Ilyaki/ProtoInput).
- Nemirtingas [OpenXinput](https://github.com/Nemirtingas/OpenXinput).
- elishacloud [DirectX-Wrappers](https://github.com/elishacloud/DirectX-Wrappers).
