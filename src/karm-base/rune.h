#pragma once

#include <karm-meta/id.h>
#include <karm-meta/traits.h>

#include "_prelude.h"

#include "array.h"
#include "buf.h"
#include "cursor.h"

namespace Karm {

using Rune = uint32_t;

template <typename T>
concept StaticEncoding = requires(T t, Rune &r, typename T::Unit u, Cursor<typename T::Unit> &c, MutCursor<typename T::Unit> &m) {
    { T::unitLen(u) } -> Meta::Same<size_t>;
    { T::runeLen(r) } -> Meta::Same<size_t>;
    { T::decodeUnit(r, c) } -> Meta::Same<bool>;
    { T::encodeUnit(Rune{}, m) } -> Meta::Same<bool>;
};

template <typename U, size_t N>
struct _Multiple {
    using Inner = U;
    using Unit = U;

    InlineBuf<Unit, N> _buf{};

    void put(Unit u) {
        _buf.emplace(_buf.len(), u);
    }

    constexpr Unit &operator[](size_t i) { return _buf[i]; }
    constexpr Unit const &operator[](size_t i) const { return _buf[i]; }

    constexpr Unit *buf() { return _buf.buf(); }
    constexpr Unit const *buf() const { return _buf.buf(); }
    constexpr size_t len() const { return _buf.len(); }
    constexpr size_t rem() const { return N - len(); }
};

template <typename U>
struct _Single {
    using Inner = U;
    using Unit = U;

    Unit _buf;

    _Single() = default;

    _Single(Unit u) : _buf(u) {}

    void put(Unit u) {
        _buf = u;
    }

    operator Unit() {
        return _buf;
    }

    constexpr Unit &operator[](size_t) { return _buf; }
    constexpr Unit const &operator[](size_t) const { return _buf; }

    constexpr Unit *buf() { return &_buf; }
    constexpr Unit const *buf() const { return &_buf; }
    constexpr size_t len() const { return 1; }
};

template <typename T, typename U>
concept EncodeOutput = requires(T t, U u) {
    {t.put(u)};
};

template <typename T, typename U>
concept DecodeInput = requires(T t, U u) {
    {t.next()};
    {t.rem()};
};

/* --- Utf8 ----------------------------------------------------------------- */

struct Utf8 {
    using Unit = char;
    using One = _Multiple<Unit, 4>;

    static constexpr size_t unitLen(Unit first) {
        if ((first & 0xf8) == 0xf0)
            return 4;
        else if ((first & 0xf0) == 0xe0)
            return 3;
        else if ((first & 0xe0) == 0xc0)
            return 2;
        else
            return 1;
    }

    static constexpr size_t runeLen(Rune rune) {
        if (rune <= 0x7f)
            return 1;
        else if (rune <= 0x7ff)
            return 2;
        else if (rune <= 0xffff)
            return 3;
        else
            return 4;
    }

    static bool decodeUnit(Rune &result, DecodeInput<Unit> auto &in) {
        if (in.rem() == 0) {
            result = U'�';
            return false;
        }

        Unit first = in.next();

        if (unitLen(first) > in.rem() + 1) {
            result = U'�';
            return false;
        }

        if ((first & 0xf8) == 0xf0) {
            result = ((0x07 & first) << 18) |
                     ((0x3f & in.next()) << 12) |
                     ((0x3f & in.next()) << 6) |
                     ((0x3f & in.next()));
        } else if ((first & 0xf0) == 0xe0) {
            result = ((0x0f & first) << 12) |
                     ((0x3f & in.next()) << 6) |
                     ((0x3f & in.next()));
        } else if ((first & 0xe0) == 0xc0) {
            result = ((0x1f & first) << 6) |
                     ((0x3f & in.next()));
        } else {
            result = first;
        }

        return true;
    }

    static bool encodeUnit(Rune c, EncodeOutput<Unit> auto &out) {
        if (unitLen(c) > out.rem())
            panic("bad");

        if (c <= 0x7f) {
            out.put(c);
        } else if (c <= 0x7ff) {
            out.put(0xc0 | (c >> 6));
            out.put(0x80 | (c & 0x3f));
        } else if (c <= 0xffff) {
            out.put(0xe0 | (c >> 12));
            out.put(0x80 | ((c >> 6) & 0x3f));
            out.put(0x80 | (c & 0x3f));
        } else if (c <= 0x10ffff) {
            out.put(0xf0 | (c >> 18));
            out.put(0x80 | ((c >> 12) & 0x3f));
            out.put(0x80 | ((c >> 6) & 0x3f));
            out.put(0x80 | (c & 0x3f));
        } else {
            return encodeUnit(U'�', out);
        }

        return true;
    }
};

[[gnu::used]] static inline Utf8 UTF8;

static_assert(StaticEncoding<Utf8>);

/* --- Utf16 ---------------------------------------------------------------- */

struct Utf16 {
    using Unit = uint16_t;
    using One = _Multiple<Unit, 2>;

    static constexpr size_t unitLen(Unit first) {
        if (first >= 0xd800 && first <= 0xdbff)
            return 2;
        else
            return 1;
    }

    static constexpr size_t runeLen(Rune rune) {
        if (rune <= 0xffff)
            return 1;
        else
            return 2;
    }

    static bool decodeUnit(Rune &result, DecodeInput<Unit> auto &in) {
        Unit first = in.next();

        if (unitLen(first) > in.rem()) {
            result = U'�';
            return false;
        }

        if (first >= 0xd800 && first <= 0xdbff) {
            if (in.rem() < 2) {
                return false;
            }

            Unit second = in.next();

            if (second < 0xdc00 || second > 0xdfff) {
                return false;
            }

            result = ((first - 0xd800) << 10) | (second - 0xdc00) + 0x10000;
        } else {
            result = first;
        }

        return true;
    }

    static bool encodeUnit(Rune c, EncodeOutput<Unit> auto &out) {
        if (c <= 0xffff) {
            out.put(c);
            return true;
        } else if (c <= 0x10ffff) {
            out.put(0xd800 | ((c - 0x10000) >> 10));
            out.put(0xdc00 | ((c - 0x10000) & 0x3ff));
            return true;
        } else {
            return encodeUnit(U'�', out);
        }
    }
};

[[gnu::used]] static inline Utf16 UTF16;

static_assert(StaticEncoding<Utf16>);

/* --- Utf32 ---------------------------------------------------------------- */

struct Utf32 {
    using Unit = char32_t;
    using One = _Single<Unit>;

    static constexpr size_t unitLen(Unit) {
        return 1;
    }

    static constexpr size_t runeLen(Rune) {
        return 1;
    }

    static bool decodeUnit(Rune &result, DecodeInput<Unit> auto &in) {
        result = in.next();
        return true;
    }

    static bool encodeUnit(Rune c, EncodeOutput<Unit> auto &out) {
        out.put(c);
        return false;
    }
};

[[gnu::used]] static inline Utf32 UTF32;

static_assert(StaticEncoding<Utf32>);

/* --- Pure ----------------------------------------------------------------- */

struct Pure {
    using Unit = Rune;
    using One = _Single<Unit>;

    static constexpr size_t unitLen(Unit) {
        return 1;
    }

    static constexpr size_t runeLen(Rune) {
        return 1;
    }

    static bool decodeUnit(Rune &result, DecodeInput<Unit> auto &in) {
        result = in.next();
        return true;
    }

    static bool encodeUnit(Rune c, EncodeOutput<Unit> auto &out) {
        out.put(c);
        return false;
    }
};

[[gnu::used]] static inline Pure PURE;

static_assert(StaticEncoding<Pure>);

/* --- Ascii ---------------------------------------------------------------- */

struct Ascii {
    using Unit = char;
    using One = _Single<Unit>;

    static constexpr size_t unitLen(Unit) {
        return 1;
    }

    static constexpr size_t runeLen(Rune) {
        return 1;
    }

    static bool decodeUnit(Rune &result, DecodeInput<Unit> auto &in) {
        auto c = in.next();
        if (c >= 0) {
            result = c;
            return true;
        } else {
            result = U'�';
            return false;
        }
    }

    static bool encodeUnit(Rune c, EncodeOutput<Unit> auto &out) {
        if (c < 0) {
            out.put('?');
            return false;
        }

        out.put(c);
        return true;
    }
};

[[gnu::used]] static inline Ascii ASCII;

static_assert(StaticEncoding<Ascii>);

/* --- Extended Ascii ------------------------------------------------------- */

template <typename Mapper>
struct EAscii {
    using Unit = uint8_t;
    using One = _Single<Unit>;

    static constexpr size_t unitLen(Unit) {
        return 1;
    }

    static constexpr size_t runeLen(Rune) {
        return 1;
    }

    static bool decodeUnit(Rune &result, DecodeInput<Unit> auto &in) {
        Mapper mapper;
        result = mapper(in.next());
        return true;
    }

    static bool encodeUnit(Rune c, EncodeOutput<Unit> auto &out) {
        Mapper mapper;
        for (size_t i = 0; i <= 255; i++) {
            if (mapper(i) == c) {
                out.put(i);
                return true;
            }
        }
        out.put('?');
        return false;
    }
};

/* --- Ibm437 --------------------------------------------------------------- */

// clang-format off

using Ibm437Mapper = decltype([](uint8_t c) {
    Array<char32_t, 256> mappings = {
        U'\0', U'☺', U'☻', U'♥', U'♦', U'♣', U'♠', U'•',
        U'◘', U'○', U'◙', U'♂', U'♀', U'♪', U'♫', U'☼',
        U'►', U'◄', U'↕', U'‼', U'¶', U'§', U'▬', U'↨',
        U'↑', U'↓', U'→', U'←', U'∟', U'↔', U'▲', U'▼',
        U' ', U'!', U'"', U'#', U'$', U'%', U'&', U'\'',
        U'(', U')', U'*', U'+', U',', U'-', U'.', U'/',
        U'0', U'1', U'2', U'3', U'4', U'5', U'6', U'7',
        U'8', U'9', U':', U';', U'<', U'=', U'>', U'?',
        U'@', U'A', U'B', U'C', U'D', U'E', U'F', U'G',
        U'H', U'I', U'J', U'K', U'L', U'M', U'N', U'O',
        U'P', U'Q', U'R', U'S', U'T', U'U', U'V', U'W',
        U'X', U'Y', U'Z', U'[', U'\\', U']', U'^', U'_',
        U'`', U'a', U'b', U'c', U'd', U'e', U'f', U'g',
        U'h', U'i', U'j', U'k', U'l', U'm', U'n', U'o',
        U'p', U'q', U'r', U's', U't', U'u', U'v', U'w',
        U'x', U'y', U'z', U'{', U'|', U'}', U'~',  U'⌂',
        U'Ç', U'ü', U'é', U'â', U'ä', U'à', U'å', U'ç',
        U'ê', U'ë', U'è', U'ï', U'î', U'ì', U'Ä', U'Å',
        U'É', U'æ', U'Æ', U'ô', U'ö', U'ò', U'û', U'ù',
        U'ÿ', U'Ö', U'Ü', U'¢', U'£', U'¥', U'₧', U'ƒ',
        U'á', U'í', U'ó', U'ú', U'ñ', U'Ñ', U'ª', U'º',
        U'¿', U'⌐', U'¬', U'½', U'¼', U'¡', U'«', U'»',
        U'░', U'▒', U'▓', U'│', U'┤', U'╡', U'╢', U'╖',
        U'╕', U'╣', U'║', U'╗', U'╝', U'╜', U'╛', U'┐',
        U'└', U'┴', U'┬', U'├', U'─', U'┼', U'╞', U'╟',
        U'╚', U'╔', U'╩', U'╦', U'╠', U'═', U'╬', U'╧',
        U'╨', U'╤', U'╥', U'╙', U'╘', U'╒', U'╓', U'╫',
        U'╪', U'┘', U'┌', U'█', U'▄', U'▌', U'▐', U'▀',
        U'α', U'ß', U'Γ', U'π', U'Σ', U'σ', U'µ', U'τ',
        U'Φ', U'Θ', U'Ω', U'δ', U'∞', U'φ', U'ε', U'∩',
        U'≡', U'±', U'≥', U'≤', U'⌠', U'⌡', U'÷', U'≈',
        U'°', U'∙', U'·', U'√', U'ⁿ', U'²', U'■', U'\x00a0',
    };

    return at(mappings, c);
});

// clang-format on

using Ibm437 = EAscii<Ibm437Mapper>;

[[gnu::used]] static inline Ibm437 IBM437;

static_assert(StaticEncoding<Ibm437>);

/* --- Latin1 --------------------------------------------------------------- */

using Latin1Mapper = decltype([](uint8_t c) {
    // HACK: """"Unicode is a "superset" of Latin1""" (please note the quotes)
    return (Rune)c;
});

using Latin1 = EAscii<Latin1Mapper>;

[[gnu::used]] static inline Latin1 LATIN1;

static_assert(StaticEncoding<Latin1>);

/* --- Utilities ------------------------------------------------------------ */

template <StaticEncoding Source, StaticEncoding Target>
size_t transcodeLen(Cursor<typename Source::Unit> input) {
    size_t result = 0;

    while (input.rem()) {
        Rune r;
        bool valid = Source::decodeUnit(r, input);
        result += Target::runeLen(valid ? r : U'�');
    }

    return result;
}

template <StaticEncoding Source>
size_t transcodeLen(Cursor<typename Source::Unit> input) {
    size_t result = 0;

    while (input.rem()) {
        Rune r;
        if (Source::decode(r, input)) {
            result += 1;
        }
    }

    return result;
}

template <StaticEncoding Source, StaticEncoding Target>
size_t transcodeUnits(Cursor<typename Source::Unit> input, MutCursor<typename Target::Unit> output) {
    size_t result = 0;

    while (input.rem()) {
        Rune r;
        bool valid = Source::decodeUnit(r, input);
        result += Target::encodeUnit(valid ? r : U'�', output);
    }

    return result;
}

template <StaticEncoding T>
using One = typename T::One;

template <StaticEncoding E>
bool encodeOne(Rune rune, One<E> &one) {
    return E::encodeUnit(rune, one);
}

template <StaticEncoding E>
bool encodeOne(One<E> &one, Rune &rune) {
    return E::decodeUnit(rune, one);
}

} // namespace Karm
