/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>
              2015 Maxim Zhurovich <zhurovich@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#ifndef BRICKS_UTIL_COMPARATOR_H
#define BRICKS_UTIL_COMPARATOR_H

#include <chrono>
#include <map>
#include <unordered_map>

namespace current {

namespace custom_comparator_and_hash_function {

template <typename T, bool HAS_MEMBER_HASH_FUNCTION, bool IS_ENUM>
struct CurrentHashFunctionImpl;

template <typename T>
struct CurrentHashFunctionImpl<T, false, false> : std::hash<T> {};

template <typename R, typename P>
struct CurrentHashFunctionImpl<std::chrono::duration<R, P>, false, false> {
  std::size_t operator()(std::chrono::duration<R, P> x) const {
    return std::hash<int64_t>()(std::chrono::duration_cast<std::chrono::microseconds>(x).count());
  }
};

template <typename T>
struct CurrentHashFunctionImpl<T, false, true> {
  std::size_t operator()(T x) const { return static_cast<size_t>(x); }
};

template <typename T>
struct CurrentHashFunctionImpl<T, true, false> {
  std::size_t operator()(const T& x) const { return x.Hash(); }
};

template <typename T, bool IS_ENUM>
struct CurrentComparatorImpl;

template <typename T>
struct CurrentComparatorImpl<T, false> : std::less<T> {};

template <typename T>
struct CurrentComparatorImpl<T, true> {
  bool operator()(T lhs, T rhs) const {
    using U = typename std::underlying_type<T>::type;
    return static_cast<U>(lhs) < static_cast<U>(rhs);
  }
};

template <typename T>
constexpr bool HasHashMethod(char) {
  return false;
}

template <typename T>
constexpr auto HasHashMethod(int) -> decltype(std::declval<const T>().Hash(), bool()) {
  return true;
}

template <typename T>
struct CurrentHashFunctionSelector {
  typedef custom_comparator_and_hash_function::CurrentHashFunctionImpl<T,
                                                                       HasHashMethod<T>(0),
                                                                       std::is_enum<T>::value> type;
};
}  // namespace custom_comparator_and_hash_function

template <typename T>
using CurrentHashFunction = typename custom_comparator_and_hash_function::CurrentHashFunctionSelector<T>::type;

template <typename T>
using CurrentComparator = custom_comparator_and_hash_function::CurrentComparatorImpl<T, std::is_enum<T>::value>;

}  // namespace current

#endif  // BRICKS_UTIL_COMPARATOR_H
