#pragma once

#include <karm-base/enum.h>
#include <karm-base/string.h>

namespace Karm::Input {

/* --- Mouse -------------------------------------------- */

enum struct Button : uint8_t {
    NONE = 0,

    LEFT = 1 << 0,
    MIDDLE = 1 << 1,
    RIGHT = 1 << 2,
    X1 = 1 << 3,
    X2 = 1 << 4,
};

FlagsEnum$(Button);

/* --- Keyboard ----------------------------------------- */

enum struct Mod : uint8_t {
    NONE = 0,
    LSHIFT = 1 << 0,
    RSHIFT = 1 << 1,
    LCTRL = 1 << 2,
    RCTRL = 1 << 3,
    ALT = 1 << 4,
    ALTGR = 1 << 5,
    SUPER = 1 << 6,
};

FlagsEnum$(Mod);

struct Key {
    enum struct Code {
#define KEY(name, code) name = code,
#include "keycodes.inc"
#undef KEY
    };

    using enum Code;

    Code _code;

    Key() = default;

    Key(Code code) : _code(code) {}

    Str name() const {
        switch (_code) {
#define KEY(name, code) \
    case Code::name:    \
        return #name;
#include "keycodes.inc"
#undef KEY
        }
        return "INVALID";
    }

    Code code() {
        return _code;
    }
};

} // namespace Karm::Input
