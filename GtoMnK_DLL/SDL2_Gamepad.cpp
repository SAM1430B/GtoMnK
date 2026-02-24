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

void SDL2_Initialize() {
    LOG("Initializing SDL2...");

    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "1");
    //SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "1");
    //SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "0");

    //SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH, "1");

    //SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");

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
        LOG("SDL2: External mapping file not found. Falling back to embedded resource.");

        HMODULE hModule = NULL;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&SDL2_Initialize,
            &hModule
        );

        HRSRC hResInfo = FindResourceA(hModule, MAKEINTRESOURCEA(IDR_TEXTFILE1), "TEXTFILE");

        if (hResInfo != NULL) {
            HGLOBAL hResData = LoadResource(hModule, hResInfo);
            void* data = LockResource(hResData);
            DWORD size = SizeofResource(hModule, hResInfo);

            if (data != NULL && size > 0) {
                SDL_RWops* rw = SDL_RWFromConstMem(data, size);
                int embeddedMappings = SDL_GameControllerAddMappingsFromRW(rw, 1);

                if (embeddedMappings > 0) {
                    LOG("SDL2: Loaded %d mappings from Embedded Resource.", embeddedMappings);
                }
                else {
                    LOG("SDL2 Error: Failed to load embedded mappings: %s", SDL_GetError());
                }
            }
        }
        else {
            LOG("SDL2 Error: Could not find the embedded resource in the DLL.");
        }
    }

    LOG("SDL2 Initialized.");
}

void SDL2_Cleanup() {
    if (g_GameController) {
        SDL_GameControllerClose(g_GameController);
        g_GameController = nullptr;
    }
    SDL_Quit();
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

bool SDL2_GetState(CustomControllerState& outState) {
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

    // Buttons
    outState.buttons[CUSTOM_ID_A] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_A);
    outState.buttons[CUSTOM_ID_B] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_B);
    outState.buttons[CUSTOM_ID_X] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_X);
    outState.buttons[CUSTOM_ID_Y] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_Y);

    outState.buttons[CUSTOM_ID_LB] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    outState.buttons[CUSTOM_ID_RB] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    outState.buttons[CUSTOM_ID_LSB] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_LEFTSTICK);
    outState.buttons[CUSTOM_ID_RSB] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_RIGHTSTICK);

    outState.buttons[CUSTOM_ID_START] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_START);
    outState.buttons[CUSTOM_ID_BACK] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_BACK);

    outState.buttons[CUSTOM_ID_DPAD_UP] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_DPAD_UP);
    outState.buttons[CUSTOM_ID_DPAD_DOWN] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    outState.buttons[CUSTOM_ID_DPAD_LEFT] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    outState.buttons[CUSTOM_ID_DPAD_RIGHT] = SDL_GameControllerGetButton(g_GameController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

    // Triggers
    Sint16 leftTrig = SDL_GameControllerGetAxis(g_GameController, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    Sint16 rightTrig = SDL_GameControllerGetAxis(g_GameController, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    outState.LeftTrigger = (BYTE)(leftTrig >> 7);
    outState.RightTrigger = (BYTE)(rightTrig >> 7);

	// Fix for SDL2 axis range being -32768 to 32767
    int leftY = -SDL_GameControllerGetAxis(g_GameController, SDL_CONTROLLER_AXIS_LEFTY);
    if (leftY > 32767) leftY = 32767;
    if (leftY < -32768) leftY = -32768;

    int rightY = -SDL_GameControllerGetAxis(g_GameController, SDL_CONTROLLER_AXIS_RIGHTY);
    if (rightY > 32767) rightY = 32767;
    if (rightY < -32768) rightY = -32768;

    // Left Stick
    outState.ThumbLX = SDL_GameControllerGetAxis(g_GameController, SDL_CONTROLLER_AXIS_LEFTX);
    outState.ThumbLY = (SHORT)leftY;

    // Right Stick
    outState.ThumbRX = SDL_GameControllerGetAxis(g_GameController, SDL_CONTROLLER_AXIS_RIGHTX);
    outState.ThumbRY = (SHORT)rightY;

    return true;
}