#pragma once
#include "InputState.h"

namespace GtoMnK {

    enum class InputMethod {
        PostMessage,
        RawInput,
        Hybrid
    };

    namespace Input {

        void SendAction(const std::string& actionString, bool press);
        void SendMouseMoveAbsolute(int screenX, int screenY);
        void SendMouseMoveDelta(int deltaX, int deltaY);

        std::vector<Action> ParseActionString(const std::string& fullString);

        const WPARAM GtoMnK_MOUSE_SIGNATURE = 0x10000000;
        const LPARAM GtoMnK_KEYBOARD_SIGNATURE = 0x10000000;

    }
}