#pragma once
#include <string>
#include "InputState.h"

namespace GtoMnK {

    enum class InputMethod {
        PostMessage,
        RawInput
    };

    namespace Input {

        void SendAction(const std::string& actionString, bool press);
        void SendAction(int screenX, int screenY);
        void SendActionDelta(int deltaX, int deltaY);

        std::vector<Action> ParseActionString(const std::string& fullString);

    }
}