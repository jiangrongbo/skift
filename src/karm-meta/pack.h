#pragma once

#include <karm-base/std.h>

#include "cvrp.h"
#include "traits.h"

namespace Karm::Meta {

template <typename T, typename... List>
concept Contains = (Same<T, List> || ...);

template <typename...>
inline constexpr size_t _indexOf = 0;

template <typename T, typename First>
inline constexpr size_t _indexOf<T, First> = 0;

template <typename T, typename First, typename... Rest>
inline constexpr size_t _indexOf<T, First, Rest...> = Same<T, First> ? 0 : _indexOf<T, Rest...> + 1;

template <typename T, typename... Ts>
requires Contains<T, Ts...>
static consteval size_t indexOf() {
    return _indexOf<T, Ts...>;
}

template <typename...>
struct _IndexCast;

template <typename Data, typename T>
struct _IndexCast<Data, T> {
    static void eval(size_t, Data *ptr, auto func) {
        using U = CopyConst<Data, T>;
        func(*reinterpret_cast<U *>(ptr));
    }
};

template <typename Data, typename First, typename... Rest>
struct _IndexCast<Data, First, Rest...> {
    static void eval(size_t index, Data *ptr, auto func) {
        using U = CopyConst<Data, First>;

        index == 0 ? func(*reinterpret_cast<U *>(ptr))
                   : _IndexCast<Data, Rest...>::eval(index - 1, ptr, func);
    }
};

template <typename... Ts>
static void indexCast(size_t index, auto *ptr, auto func) {
    _IndexCast<RemoveRef<decltype(*ptr)>, Ts...>::eval(index, ptr, func);
}

} // namespace Karm::Meta
