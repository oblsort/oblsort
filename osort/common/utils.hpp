#pragma once

#include <vector>

#include "common/defs.hpp"
#include "common/encutils.hpp"
#include "common/mov_intrinsics.hpp"

constexpr INLINE uint64_t GetNextPowerOfTwo(uint64_t n) {
  Assert(n <= 0x8000'0000'0000'0000);
  n = n - 1;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  n = n + 1;
  return n;
}

INLINE uint64_t GetLogBaseTwo(uint64_t n) {
  const uint64_t masks[6] = {0x2,    0xC,         0xF0,
                             0xFF00, 0xFFFF'0000, 0xFFFF'FFFF'0000'0000};
  uint64_t c = 32;
  uint64_t r = 0;
  for (int32_t i = 5; i >= 0; i--) {
    const bool cond = n & masks[i];
    obliMove(cond, n, n >> c);
    obliMove(cond, r, r | c);
    c >>= 1;
  }
  return r;
}

INLINE uint64_t CeilLog2(uint64_t x) {
  static const uint64_t t[6] = {0xFFFFFFFF00000000ull, 0x00000000FFFF0000ull,
                                0x000000000000FF00ull, 0x00000000000000F0ull,
                                0x000000000000000Cull, 0x0000000000000002ull};

  uint64_t y = (((x & (x - 1)) == 0) ? 0 : 1);
  int j = 32;
  int i;

  for (i = 0; i < 6; i++) {
    int k = (((x & t[i]) == 0) ? 0 : j);
    y += k;
    x >>= k;
    j >>= 1;
  }

  return y;
}

extern RandGen default_rand;

// [left,right]
inline uint64_t UniformRandom(uint64_t left, uint64_t right) {
  return default_rand.rand64() % (right - left + 1) + left;
}

inline uint32_t UniformRandom32(uint32_t left, uint32_t right) {
  return default_rand.rand32() % (right - left + 1) + left;
}

inline bool UniformRandomBit() { return default_rand.rand1(); }

// [0,right]
INLINE uint64_t UniformRandom(uint64_t right) {
  return UniformRandom(0, right);
}

// [0,right]
INLINE uint64_t UniformRandom() { return default_rand.rand64(); }

INLINE uint64_t UniformRandom32(uint32_t right) {
  return UniformRandom32(0, right);
}

// [0,right]
INLINE uint64_t UniformRandom32() { return default_rand.rand32(); }

INLINE void GetRand16(uint8_t* out) {
  *(uint64_t*)out = UniformRandom();
  *(uint64_t*)(out + 8) = UniformRandom();
}

// x/y round up
INLINE uint64_t divRoundUp(size_t x, size_t y) { return (x + y - 1) / y; }

template <typename Iterator>
static auto getVecProduct(Iterator begin, Iterator end) {
  using NumType = typename std::iterator_traits<Iterator>::value_type;
  NumType product = 1;
  for (auto it = begin; it != end; ++it) {
    product *= *it;
  }
  return product;
}

template <typename Vec>
static auto getVecProduct(const Vec& vec) {
  return getVecProduct(vec.begin(), vec.end());
}

/**
 * Note: this function is not oblivious
 */
template <typename Iterator>
void fisherYatesShuffle(Iterator begin, Iterator end) {
  size_t N = end - begin;
  for (size_t n = N - 1; n; --n) {
    size_t randPos = UniformRandom(n);
    std::swap(*(begin + randPos), *(--end));
  }
}