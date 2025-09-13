#pragma once
#include <string>
#include "InputState.h"

namespace ScreenshotInput {

    enum class SendMethod {
        PostMessage,
        SendInput
    };

    namespace Input {

        void SendAction(const std::string& actionString, bool press);
        void SendAction(int screenX, int screenY);

        std::vector<Action> ParseActionString(const std::string& fullString);

    }
}