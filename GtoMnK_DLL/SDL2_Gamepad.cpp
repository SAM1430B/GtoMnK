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

extern bool enableSDL2Mask;
bool g_IsParasiticMode = false;

extern int mode;
extern int righthanded;

void SDL2_Initialize() {
    LOG("Initializing SDL2...");

    HMODULE hGameSDL2 = GetModuleHandleA("SDL2.dll");
    if (hGameSDL2 != NULL) {
        LOG("Game uses SDL2.dll. Entering Passive/Parasitic Mode.");
        g_IsParasiticMode = true;
        return;
    }

    LOG("Game does NOT use SDL2.dll. Entering Standalone Mode.");
    g_IsParasiticMode = false;

    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    //SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "1");
    //SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "1");
    //SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "0");

    //SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH, "1");

    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) < 0) {
        LOG("Static SDL2 Init Failed: %s", SDL_GetError());
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
    LOG("Static SDL2 Initialized.");
}

void SDL2_Cleanup() {
    if (g_IsParasiticMode) {
        g_GameController = nullptr;
    }
    else {
        if (g_GameController) {
            SDL_GameControllerClose(g_GameController);
            g_GameController = nullptr;
        }
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
    }
}

// Memory-Safe Readers
Uint8 SafeReadButton(SDL_GameController* gc, SDL_GameControllerButton button) {
    if (g_IsParasiticMode && TrueSDLGetButton != nullptr) {
        return TrueSDLGetButton(gc, button); // Read from Game's DLL
    }
    return SDL_GameControllerGetButton(gc, button); // Read from GtoMnK Static lib
}

Sint16 SafeReadAxis(SDL_GameController* gc, SDL_GameControllerAxis axis) {
    if (g_IsParasiticMode && TrueSDLGetAxis != nullptr) {
        return TrueSDLGetAxis(gc, axis); // Read from Game's DLL
    }
    return SDL_GameControllerGetAxis(gc, axis); // Read from GtoMnK Static lib
}

bool SDL2_GetState(CustomControllerState& outState) {
    if (g_IsParasiticMode) {
        if (!g_GameController || !TrueSDLGetButton || !TrueSDLGetAxis) return false;
    }
    else {
        if (g_GameController && SDL_GameControllerGetAttached(g_GameController) == SDL_FALSE) {
            SDL_GameControllerClose(g_GameController);
            g_GameController = nullptr;
            g_JoyID = -1;
        }

        if (!g_GameController) {
            static ULONGLONG lastConnectAttempt = 0;
            if (GetTickCount64() - lastConnectAttempt > 1000) {
                lastConnectAttempt = GetTickCount64();
                int joyCount = SDL_NumJoysticks();
                for (int i = 0; i < joyCount; ++i) {
                    if (SDL_IsGameController(i)) {
                        SDL_GameController* tempController = SDL_GameControllerOpen(i);
                        if (tempController) {
                            SDL_Joystick* joy = SDL_GameControllerGetJoystick(tempController);
                            const char* newSerial = SDL_JoystickGetSerial(joy);
                            const char* newPath = SDL_JoystickPath(joy);
                            bool isMatch = false;

                            if (g_JoySerialNum.empty() && g_JoyHardwarePath.empty()) {
                                if (i == controllerID) isMatch = true;
                            }
                            else {
                                if (!g_JoySerialNum.empty() && newSerial != nullptr) isMatch = (g_JoySerialNum == newSerial);
                                else if (!g_JoyHardwarePath.empty() && newPath != nullptr) isMatch = (g_JoyHardwarePath == newPath);
                            }

                            if (isMatch) {
                                g_GameController = tempController;
                                g_JoyID = SDL_JoystickInstanceID(joy);
                                if (g_JoySerialNum.empty() && g_JoyHardwarePath.empty()) {
                                    if (newSerial) g_JoySerialNum = newSerial;
                                    if (newPath) g_JoyHardwarePath = newPath;
                                }
                                break;
                            }
                            else {
                                SDL_GameControllerClose(tempController);
                            }
                        }
                    }
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

Uint8 SDLCALL Hook_SDL_GameControllerGetButton(SDL_GameController* gamecontroller, SDL_GameControllerButton button) {
    g_GameController = gamecontroller;

    UINT customID = 0;
    switch (button) {
    case SDL_CONTROLLER_BUTTON_A: customID = CUSTOM_ID_A; break;
    case SDL_CONTROLLER_BUTTON_B: customID = CUSTOM_ID_B; break;
    case SDL_CONTROLLER_BUTTON_X: customID = CUSTOM_ID_X; break;
    case SDL_CONTROLLER_BUTTON_Y: customID = CUSTOM_ID_Y; break;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: customID = CUSTOM_ID_LB; break;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: customID = CUSTOM_ID_RB; break;
    case SDL_CONTROLLER_BUTTON_LEFTSTICK: customID = CUSTOM_ID_LSB; break;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK: customID = CUSTOM_ID_RSB; break;
    case SDL_CONTROLLER_BUTTON_START: customID = CUSTOM_ID_START; break;
    case SDL_CONTROLLER_BUTTON_BACK: customID = CUSTOM_ID_BACK; break;
    case SDL_CONTROLLER_BUTTON_DPAD_UP: customID = CUSTOM_ID_DPAD_UP; break;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: customID = CUSTOM_ID_DPAD_DOWN; break;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: customID = CUSTOM_ID_DPAD_LEFT; break;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: customID = CUSTOM_ID_DPAD_RIGHT; break;
    }

    if (enableSDL2Mask && customID != 0 && IsButtonMapped(customID)) {
        return 0;
    }

    if (TrueSDLGetButton) return TrueSDLGetButton(gamecontroller, button);
    return 0;
}

Sint16 SDLCALL Hook_SDL_GameControllerGetAxis(SDL_GameController* gamecontroller, SDL_GameControllerAxis axis) {
    g_GameController = gamecontroller;

    bool mapped = false;

    if (mode == 1) {
        if (righthanded == 1 && (axis == SDL_CONTROLLER_AXIS_RIGHTX || axis == SDL_CONTROLLER_AXIS_RIGHTY)) {
            mapped = true; // Hide Right Stick
        }
        else if (righthanded == 0 && (axis == SDL_CONTROLLER_AXIS_LEFTX || axis == SDL_CONTROLLER_AXIS_LEFTY)) {
            mapped = true; // Hide Left Stick
        }
    }

    // Mask sticks and triggers if mapped as buttons
    if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT && IsButtonMapped(CUSTOM_ID_LT)) mapped = true;
    if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT && IsButtonMapped(CUSTOM_ID_RT)) mapped = true;
    if (axis == SDL_CONTROLLER_AXIS_LEFTX && (IsButtonMapped(CUSTOM_ID_LSL) || IsButtonMapped(CUSTOM_ID_LSR))) mapped = true;
    if (axis == SDL_CONTROLLER_AXIS_LEFTY && (IsButtonMapped(CUSTOM_ID_LSU) || IsButtonMapped(CUSTOM_ID_LSD))) mapped = true;
    if (axis == SDL_CONTROLLER_AXIS_RIGHTX && (IsButtonMapped(CUSTOM_ID_RSL) || IsButtonMapped(CUSTOM_ID_RSR))) mapped = true;
    if (axis == SDL_CONTROLLER_AXIS_RIGHTY && (IsButtonMapped(CUSTOM_ID_RSU) || IsButtonMapped(CUSTOM_ID_RSD))) mapped = true;

    if (enableSDL2Mask && mapped) {
        return 0;
    }

    if (TrueSDLGetAxis) return TrueSDLGetAxis(gamecontroller, axis);
    return 0;
}