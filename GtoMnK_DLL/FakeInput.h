#pragma once
#include "pch.h"
#include <string>
#include <vector>

namespace GtoMnK {

    enum class FakeInputMethod {
        PostMessage,
        RawInput,
        Hybrid
    };

    struct FakeInputAction {
        std::string actionString = "0";
        bool onRelease = false;
        int holdDurationMs = 0;
    };

    namespace FakeInput {

        void SendAction(const std::string& actionString, bool press);
        void SendMouseMoveAbsolute(int screenX, int screenY);
        void SendMouseMoveDelta(int deltaX, int deltaY);

        std::vector<FakeInputAction> ParseActionString(const std::string& fullString);

        const WPARAM GtoMnK_MOUSE_SIGNATURE = 0x10000000;
        const LPARAM GtoMnK_KEYBOARD_SIGNATURE = 0x10000000;

    }
}