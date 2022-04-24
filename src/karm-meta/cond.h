#pragma once

namespace Karm::Meta
{

template <bool cond, typename TTrue, typename TFalse>
struct _Cond
{
    using Type = TTrue;
};

template <typename TTrue, typename TFalse>
struct _Cond<false, TTrue, TFalse>
{
    using Type = TFalse;
};

template <bool cond, typename TTrue, typename TFalse>
using Cond = typename _Cond<cond, TTrue, TFalse>::Type;

} // namespace Karm::Meta
