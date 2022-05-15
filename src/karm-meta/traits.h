#pragma once

namespace Karm::Meta {

template <typename T>
concept Trivial = __is_trivial(T);

template <typename Derived, typename Base>
concept Derive = __is_base_of(Base, Derived);

template <typename T, typename U>
inline constexpr bool _Same = false;

template <typename T>
inline constexpr bool _Same<T, T> = true;

template <typename T, typename U>
concept Same = _Same<T, U>;

} // namespace Karm::Meta
