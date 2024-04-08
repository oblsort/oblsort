#pragma once
#include "apps_common.hpp"

namespace Apps {
using namespace EM::Algorithm;
template <typename T>
struct HistEntry {
  T key;
  uint64_t count = 0;
  bool operator<(const HistEntry& other) const {
    return ((key < other.key) | (!other.count)) & !!count;
  }
};

template <const SortMethod method, class IntermediateVec, class Reader,
          class Writer>
void histogramHelper(uint64_t size, Reader& reader, Writer& writer) {
  using T = typename Reader::value_type;
  using HistEntry_ = HistEntry<T>;
  IntermediateVec hist(size);
  typename IntermediateVec::Writer histWriter(hist.begin(), hist.end());
  uint64_t dupCounter = 0;
  T prev;
  EM::VirtualVector::VirtualWriter<T> vWriter(
      size, [&](uint64_t idx, const T& ele) {
        if (idx == 0) {
          prev = ele;
          return;
        }
        bool isDup = prev == ele;
        ++dupCounter;
        uint64_t toWriteFreq = dupCounter;
        obliMove(isDup, toWriteFreq, 0UL);  // dummy element
        // std::cout << "write " << prev << " " << toWriteFreq << std::endl;
        histWriter.write(HistEntry_{prev, toWriteFreq});
        obliMove(!isDup, dupCounter, 0UL);
        prev = ele;
      });
  if constexpr (method == KWAYBUTTERFLYOSORT) {
    KWayButterflySort(reader, vWriter, DEFAULT_HEAP_SIZE);
    histWriter.write(HistEntry_{prev, ++dupCounter});
    // std::cout << "write " << prev << " " << dupCounter << std::endl;
    typename IntermediateVec::Reader histReader(hist.begin(), hist.end());
    KWayButterflySort(histReader, writer, DEFAULT_HEAP_SIZE);
  } else if constexpr (method == BITONICSORT) {
    BitonicSortRW(reader, vWriter);
    histWriter.write(HistEntry_{prev, ++dupCounter});
    // std::cout << "write " << prev << " " << dupCounter << std::endl;
    typename IntermediateVec::Reader histReader(hist.begin(), hist.end());
    BitonicSortRW(histReader, writer);
  }
}
template <const SortMethod method, class Reader, class Writer>
void histogram(Reader& reader, Writer& writer) {
  uint64_t size = reader.size();
  using T = typename Reader::value_type;
  using HistEntry_ = HistEntry<T>;
  static_assert(method == KWAYBUTTERFLYOSORT || method == BITONICSORT,
                "Invalid method for histogram");

  if (DEFAULT_HEAP_SIZE < sizeof(HistEntry_) * size * 2) {  // external memory
    using IntermediateVec =
        EM::NonCachedVector::Vector<HistEntry_, (1UL << 16)>;
    histogramHelper<method, IntermediateVec>(size, reader, writer);
  } else {
    using IntermediateVec = StdVector<HistEntry_>;
    histogramHelper<method, IntermediateVec>(size, reader, writer);
  }
}
}  // namespace Apps