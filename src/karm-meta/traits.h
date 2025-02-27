#pragma once

namespace Karm::Meta {

/* --- Primary type categories ---------------------------------------------- */

template <typename T>
concept Void = __is_void(T);

template <typename T>
inline constexpr bool _Nullptr = false;

template <>
inline constexpr bool _Nullptr<decltype(nullptr)> = true;

template <typename T>
concept Nullptr = _Nullptr<T>;

template <typename T>
concept Float = __is_floating_point(T);

template <typename T>
concept Array = __is_array(T);

template <typename T>
concept Enum = __is_enum(T);

template <typename T>
concept Union = __is_union(T);

template <typename T>
concept Struct = __is_class(T);

template <typename T>
concept Integral = __is_integral(T);

/* --- Type properties ------------------------------------------------------ */

template <typename T>
concept Trivial = __is_trivial(T);

template <typename T>
concept Signed = __is_signed(T);

template <typename T>
concept Unsigned = __is_unsigned(T);

template <typename T>
concept SignedIntegral = Integral<T> && Signed<T>;

template <typename T>
concept UnsignedIntegral = Integral<T> && Unsigned<T>;

template <typename T>
using UnderlyingType = __underlying_type(T);

/* --- Type relationships --------------------------------------------------- */

template <typename Derived, typename Base>
concept Derive = __is_base_of(Base, Derived);

template <typename T, typename U>
inline constexpr bool _Same = false;

template <typename T>
inline constexpr bool _Same<T, T> = true;

template <typename T, typename U>
concept Same = _Same<T, U>;

} // namespace Karm::Meta
