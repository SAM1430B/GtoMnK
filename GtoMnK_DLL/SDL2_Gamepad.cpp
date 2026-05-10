#include "pch.h"
#include "SDL2_Gamepad.h"
#include "Logging.h"
#include "INISettings.h"
#include <string>
#include "resource.h"

SDL_GameController* g_GameController = nullptr;
SDL_JoystickID g_JoyID = -1;

std::string g_JoySerialNum = "";
std::string g_JoyHardwarePath = "";

// Pointers to the GAME'S dynamic SDL2.dll functions
SDL_GameControllerGetButton_t TrueSDLGetButton = nullptr;
SDL_GameControllerGetAxis_t TrueSDLGetAxis = nullptr;

bool g_IsParasiticMode = false;

typedef int(SDLCALL* SDL_NumJoysticks_t)(void);
typedef SDL_bool(SDLCALL* SDL_IsGameController_t)(int);
typedef SDL_GameController* (SDLCALL* SDL_GameControllerOpen_t)(int);

SDL_NumJoysticks_t TrueSDLNumJoysticks = nullptr;
SDL_IsGameController_t TrueSDLIsGameController = nullptr;
SDL_GameControllerOpen_t TrueSDLGameControllerOpen = nullptr;

void SDL2_Initialize() {
    LOG("Initializing SDL2...");

    HMODULE hGameSDL2 = GetModuleHandleA("SDL2.dll");
    if (hGameSDL2 != NULL) {
        LOG("Game uses SDL2.dll. Entering Passive/Parasitic Mode.");
        g_IsParasiticMode = true;

        TrueSDLGetButton = (SDL_GameControllerGetButton_t)GetProcAddress(hGameSDL2, "SDL_GameControllerGetButton");
        TrueSDLGetAxis = (SDL_GameControllerGetAxis_t)GetProcAddress(hGameSDL2, "SDL_GameControllerGetAxis");
        TrueSDLNumJoysticks = (SDL_NumJoysticks_t)GetProcAddress(hGameSDL2, "SDL_NumJoysticks");
        TrueSDLIsGameController = (SDL_IsGameController_t)GetProcAddress(hGameSDL2, "SDL_IsGameController");
        TrueSDLGameControllerOpen = (SDL_GameControllerOpen_t)GetProcAddress(hGameSDL2, "SDL_GameControllerOpen");
        return;
    }

    LOG("Game does NOT use SDL2.dll. Entering Standalone Mode.");
    g_IsParasiticMode = false;

    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "1");
    //SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "1");
    //SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "0");

    //SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH, "1");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) < 0) {
        LOG("SDL2 Init Failed: %s", SDL_GetError());
        return;
    }

    int mappingsAdded = SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

    // Load gamecontrollerdb.txt if it exists.
    if (mappingsAdded > 0) {
        LOG("SDL2: Loaded %d mappings from external 'gamecontrollerdb.txt'.", mappingsAdded);
    }
    else {
        HMODULE hModule = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&SDL2_Initialize, &hModule);
        HRSRC hResInfo = FindResourceA(hModule, MAKEINTRESOURCEA(IDR_TEXTFILE1), "TEXTFILE");
        if (hResInfo != NULL) {
            HGLOBAL hResData = LoadResource(hModule, hResInfo);
            void* data = LockResource(hResData);
            DWORD size = SizeofResource(hModule, hResInfo);
            if (data != NULL && size > 0) {
                SDL_RWops* rw = SDL_RWFromConstMem(data, size);
                int embeddedMappings = SDL_GameControllerAddMappingsFromRW(rw, 1);
                if (embeddedMappings > 0) LOG("SDL2: Loaded %d mappings from Embedded Resource.", embeddedMappings);
            }
        }
    }
    LOG("SDL2 Initialized.");
}

void SDL2_Cleanup() {
    if (g_GameController && !g_IsParasiticMode) {
        SDL_GameControllerClose(g_GameController);
        g_GameController = nullptr;
    }
    if (!g_IsParasiticMode) {
        SDL_Quit();
    }
}

// Opens the SDL_GameController at the given virtual index (skipping non-gamepad devices).
SDL_GameController* OpenControllerByVirtualIndex(int virtualIndex) {
    int joyCount = SDL_NumJoysticks();
    int foundGamepads = 0;

    for (int i = 0; i < joyCount; ++i) {
        if (!SDL_IsGameController(i)) continue;

        if (foundGamepads == virtualIndex) {
            return SDL_GameControllerOpen(i);
        }
        foundGamepads++;
    }
    return nullptr;
}

void AttemptInitialConnect() {
    if (g_GameController) return;

    g_GameController = OpenControllerByVirtualIndex(controllerID);

    // To keep track of the specific used controller.
    if (g_GameController) {
        SDL_Joystick* joy = SDL_GameControllerGetJoystick(g_GameController);
        g_JoyID = SDL_JoystickInstanceID(joy);

        // Serial Number (USB port changes)
        const char* serial = SDL_JoystickGetSerial(joy);
        if (serial) {
            g_JoySerialNum = serial;
        }

        // OS Device Path (Fallback, port-dependent)
        const char* path = SDL_JoystickPath(joy);
        if (path) {
            g_JoyHardwarePath = path;
        }
        LOG("SDL2: Using %s",
            SDL_GameControllerName(g_GameController));

        LOG("SDL2: Joystick Serial: [%s]",
            g_JoySerialNum.empty() ? "NONE" : g_JoySerialNum.c_str());
        LOG("SDL2: Joystick Path: [%s]",
            g_JoyHardwarePath.empty() ? "NONE" : g_JoyHardwarePath.c_str());

    }
}

// Memory-Safe Readers
Uint8 SafeReadButton(SDL_GameController* gc, SDL_GameControllerButton button) {
    if (g_IsParasiticMode && TrueSDLGetButton != nullptr) return TrueSDLGetButton(gc, button);
    return SDL_GameControllerGetButton ? SDL_GameControllerGetButton(gc, button) : 0;
}

Sint16 SafeReadAxis(SDL_GameController* gc, SDL_GameControllerAxis axis) {
    if (g_IsParasiticMode && TrueSDLGetAxis != nullptr) return TrueSDLGetAxis(gc, axis);
    return SDL_GameControllerGetAxis ? SDL_GameControllerGetAxis(gc, axis) : 0;
}

bool SDL2_GetState(CustomControllerState& outState) {
    if (g_IsParasiticMode) {
        if (!g_GameController && TrueSDLNumJoysticks && TrueSDLIsGameController && TrueSDLGameControllerOpen) {
            int joyCount = TrueSDLNumJoysticks();
            for (int i = 0; i < joyCount; ++i) {
                if (TrueSDLIsGameController(i)) {
                    g_GameController = TrueSDLGameControllerOpen(i);
                    if (g_GameController) break;
                }
            }
        }
    }
    else {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_CONTROLLERDEVICEADDED) {
                int deviceIndex = event.cdevice.which;

                if (!g_GameController) {
                    // If never successfully connected before, try controllerID
                    if (g_JoySerialNum.empty() && g_JoyHardwarePath.empty()) {
                        AttemptInitialConnect();
                    }
                    else {
                        SDL_GameController* tempController = SDL_GameControllerOpen(deviceIndex);

                        if (tempController) {
                            SDL_Joystick* joy = SDL_GameControllerGetJoystick(tempController);
                            const char* newSerial = SDL_JoystickGetSerial(joy);
                            const char* newPath = SDL_JoystickPath(joy);
                            bool isMatch = false;

                            // The priority is the serial numbers match.
                            if (!g_JoySerialNum.empty() && newSerial != nullptr) {
                                isMatch = (g_JoySerialNum == newSerial);
                            }
                            // If no serial is available. Check the USB path match.
                            else if (!g_JoyHardwarePath.empty() && newPath != nullptr) {
                                isMatch = (g_JoyHardwarePath == newPath);
                            }

                            // Keep it if it's the matched controller. 
                            if (isMatch) {
                                g_GameController = tempController;
                                g_JoyID = SDL_JoystickInstanceID(joy);
                                LOG("SDL2: Controller is successfully reclaimed!");
                            }
                            else {
                                // Not the matched controller
                                SDL_GameControllerClose(tempController);
                            }
                        }
                    }
                }
            }
            else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                // Disconnect if the device removed.
                if (g_GameController && event.cdevice.which == g_JoyID) {
                    SDL_GameControllerClose(g_GameController);
                    g_GameController = nullptr;
                    g_JoyID = -1;

                    LOG("SDL2: Controller is disconnected. Waiting for original hardware...");
                }
            }
        }
        if (!g_GameController) return false;
    }

    // Buttons
    outState.buttons[CUSTOM_ID_A] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_A);
    outState.buttons[CUSTOM_ID_B] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_B);
    outState.buttons[CUSTOM_ID_X] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_X);
    outState.buttons[CUSTOM_ID_Y] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_Y);
    outState.buttons[CUSTOM_ID_LB] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    outState.buttons[CUSTOM_ID_RB] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    outState.buttons[CUSTOM_ID_LSB] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_LEFTSTICK);
    outState.buttons[CUSTOM_ID_RSB] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    outState.buttons[CUSTOM_ID_START] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_START);
    outState.buttons[CUSTOM_ID_BACK] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_BACK);
    outState.buttons[CUSTOM_ID_DPAD_UP] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_DPAD_UP);
    outState.buttons[CUSTOM_ID_DPAD_DOWN] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    outState.buttons[CUSTOM_ID_DPAD_LEFT] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    outState.buttons[CUSTOM_ID_DPAD_RIGHT] = SafeReadButton(g_GameController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

    // Triggers
    Sint16 leftTrig = SafeReadAxis(g_GameController, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    Sint16 rightTrig = SafeReadAxis(g_GameController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    outState.LeftTrigger = (BYTE)(leftTrig >> 7);
    outState.RightTrigger = (BYTE)(rightTrig >> 7);

    // Fix for SDL2 axis range being -32768 to 32767
    int leftY = -SafeReadAxis(g_GameController, SDL_CONTROLLER_AXIS_LEFTY);
    if (leftY > 32767) leftY = 32767;
    if (leftY < -32768) leftY = -32768;

    int rightY = -SafeReadAxis(g_GameController, SDL_CONTROLLER_AXIS_RIGHTY);
    if (rightY > 32767) rightY = 32767;
    if (rightY < -32768) rightY = -32768;

    // Sticks
    outState.ThumbLX = SafeReadAxis(g_GameController, SDL_CONTROLLER_AXIS_LEFTX);
    outState.ThumbLY = (SHORT)leftY;
    outState.ThumbRX = SafeReadAxis(g_GameController, SDL_CONTROLLER_AXIS_RIGHTX);
    outState.ThumbRY = (SHORT)rightY;
        
    return true;
}