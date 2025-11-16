#pragma once

namespace GtoMnK {
    template<unsigned int FilterId, unsigned int MsgMin, unsigned int MsgMax = MsgMin>
    struct MessageFilterBase {
        constexpr static unsigned int FilterID() { return FilterId; }
        constexpr static unsigned int MessageMin() { return MsgMin; }
        constexpr static unsigned int MessageMax() { return MsgMax; }
    };
}