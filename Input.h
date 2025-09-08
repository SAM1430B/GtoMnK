#pragma once
#include <string>

namespace ScreenshotInput {

    enum class SendMethod {
        PostMessage,
        SendInput
    };

    namespace Input {

        void SendAction(const std::string& actionString, bool press);

        void SendAction(int screenX, int screenY);

    }
}