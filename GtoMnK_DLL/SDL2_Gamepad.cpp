#include "pch.h"
#include "SDL2_Gamepad.h"
#include "Logging.h"
#include "INISettings.h"
#include "SDL2_gamecontrollerdb.h"
#include <string>

SDL_GameController* g_GameController = nullptr;

// Opens the SDL_GameController at the given virtual index (skipping non-gamepad devices).
SDL_GameController* OpenControllerByVirtualIndex(int virtualIndex) {
    int joyCount = SDL_NumJoysticks();
    int foundGamepads = 0;

    for (int i = 0; i < joyCount; ++i) {
        if (!SDL_IsGameController(i)) {
            continue;
        }

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
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_SWITCH, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_XBOX, "0");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) < 0) {
        LOG("SDL2 Init Failed: %s", SDL_GetError());
        return;
    }

	// Load gamecontrollerdb.txt if it exists.
    int mappingsAdded = SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

    if (mappingsAdded > 0) {
        LOG("SDL2: Loaded %d mappings from external 'gamecontrollerdb.txt'.", mappingsAdded);
    }
	else { // Fallback to embedded database
        LOG("SDL2: External mapping file not found. Using embedded database.");

        int totalEmbedded = 0;
        for (int i = 0; EMBEDDED_MAPPINGS_PARTS[i] != nullptr; i++) {
            SDL_RWops* rw = SDL_RWFromConstMem(EMBEDDED_MAPPINGS_PARTS[i], (int)strlen(EMBEDDED_MAPPINGS_PARTS[i]));
            int chunkAdded = SDL_GameControllerAddMappingsFromRW(rw, 1);
            if (chunkAdded > 0) totalEmbedded += chunkAdded;
        }

        if (totalEmbedded > 0) {
            LOG("SDL2: Loaded %d mappings from Embedded Database.", totalEmbedded);
        }
        else {
            LOG("SDL2 Error: Failed to load embedded mappings: %s", SDL_GetError());
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

void AttemptConnect() {
    // Check connection
    if (g_GameController) {
        if (!SDL_GameControllerGetAttached(g_GameController)) {
            SDL_GameControllerClose(g_GameController);
            g_GameController = nullptr;
            LOG("SDL2: Controller Disconnected.");
        }
    }

    // If disconnected
    if (!g_GameController) {
        static ULONGLONG lastLogTime = 0;
        ULONGLONG currentTime = GetTickCount64();
        bool shouldLog = (currentTime - lastLogTime > 3000);

        SDL_PumpEvents();

        g_GameController = OpenControllerByVirtualIndex(controllerID);

        if (g_GameController) {
            LOG("SDL2: Connected to Slot %d: %s", controllerID, SDL_GameControllerName(g_GameController));
        }
        else if (shouldLog) {
            LOG("SDL2: Searching for controller in Slot %d... (Total Devices: %d)", controllerID, SDL_NumJoysticks());
            lastLogTime = currentTime;
        }
    }
}

bool SDL2_GetState(CustomControllerState& outState) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (g_GameController && !SDL_GameControllerGetAttached(g_GameController)) {
                SDL_GameControllerClose(g_GameController);
                g_GameController = nullptr;
            }
        }
    }

    AttemptConnect();

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