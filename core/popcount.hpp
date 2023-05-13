#pragma once

#include <bitset>
#include <numeric>
#include <type_traits>

namespace core {

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, unsigned>::type
    popcount(T x) {
        static_assert(std::numeric_limits<T>::radix == 2, "non-binary type");
        constexpr int bitwidth = std::numeric_limits<T>::digits + std::numeric_limits<T>::is_signed;
        static_assert(bitwidth <= std::numeric_limits<unsigned long long>::digits, "arg too wide for std::bitset() constructor");
        typedef typename std::make_unsigned<T>::type UT;
        std::bitset<bitwidth> bs(static_cast<UT>(x));
        return bs.count();
    }

} // namespace core