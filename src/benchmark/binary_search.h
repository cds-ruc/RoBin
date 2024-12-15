#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
__attribute__((always_inline)) inline uint32_t bsr(uint32_t x) {
  return 31 - __builtin_clz(x);
}
__attribute__((always_inline)) inline uint64_t bsr(uint64_t x) {
  return 63 - __builtin_clzll(x);
}

template <class Tp>
constexpr inline size_t binary_search_lower_bound_pow2(const Tp *arr, size_t n,
                                                       Tp key) {
  assert((n & (n + 1)) == 0); // n = 2^k - 1
  size_t pos = -1;
  size_t step = 1UL << bsr(n);
  while (step > 0) {
    pos += (arr[pos + step] < key) * step;
    step >>= 1;
  }
  return pos + 1;
}
template <class Tp>
constexpr inline size_t binary_search_upper_bound_pow2(const Tp *arr, size_t n,
                                                       Tp key) {
  assert((n & (n + 1)) == 0); // n = 2^k - 1
  size_t pos = -1;
  size_t step = 1UL << bsr(n);
  while (step > 0) {
    pos += (arr[pos + step] <= key) * step;
    step >>= 1;
  }
  return pos + 1;
}
template <class Tp>
constexpr inline size_t binary_search_lower_bound(const Tp *arr, size_t n,
                                                  Tp key) {
  const Tp *begin = arr;
  // while length isn't a power of 2 minus 1
  while (n & (n + 1)) {
    size_t step = n / 8 * 6 + 1;
    if (begin[step] < key) {
      begin += step + 1;
      n -= step + 1;
    } else {
      n = step;
      break;
    }
  }
  while (n != 0) {
    n /= 2;
    if (begin[n] < key) {
      begin += n + 1;
    }
  }
  return begin - arr;
}
template <class Tp>
constexpr inline size_t binary_search_upper_bound(const Tp *arr, size_t n,
                                                  Tp key) {
  const Tp *begin = arr;
  while (n & (n + 1)) {
    size_t step = n / 8 * 6 + 1;
    if (begin[step] <= key) {
      begin += step + 1;
      n -= step + 1;
    } else {
      n = step;
      break;
    }
  }
  while (n != 0) {
    n /= 2;
    if (begin[n] <= key) {
      begin += n + 1;
    }
  }
  return begin - arr;
}

template <class Tp>
constexpr inline size_t heuristic_lower_bound(const Tp *arr, size_t n, Tp key) {
  if ((n & (n + 1)) == 0) {
    // branchless binary search
    return binary_search_lower_bound_pow2(arr, n, key);
  }
  return binary_search_lower_bound(arr, n, key);
}

template <class ForwardIterator, class Tp>
constexpr inline ForwardIterator heuristic_lower_bound(ForwardIterator first,
                                                       ForwardIterator last,
                                                       const Tp &value) {
  size_t length = last - first;
  if ((length & (length + 1)) == 0) {
    // branchless binary search
    return first + binary_search_lower_bound_pow2(&(*first), length, value);
  }
  return first + binary_search_lower_bound(&(*first), length, value);
}

template <class Tp>
constexpr inline size_t heuristic_upper_bound(const Tp *arr, size_t n, Tp key) {
  if ((n & (n + 1)) == 0) {
    // branchless binary search
    return binary_search_upper_bound_pow2(arr, n, key);
  }
  return binary_search_upper_bound(arr, n, key);
}

template <class ForwardIterator, class Tp>
constexpr inline ForwardIterator heuristic_upper_bound(ForwardIterator first,
                                                       ForwardIterator last,
                                                       const Tp &value) {
  size_t length = last - first;
  if ((length & (length + 1)) == 0) {
    // branchless binary search
    return first + binary_search_upper_bound_pow2(&(*first), length, value);
  }
  return first + binary_search_upper_bound(&(*first), length, value);
}
