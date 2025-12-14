#pragma once

struct Action {
    std::string actionString = "0";
    bool onRelease = false;
    int holdDurationMs = 0;
};