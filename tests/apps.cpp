#include <gtest/gtest.h>

#include "apps/histogram.hpp"
#include "apps/oram_init.hpp"
using namespace Apps;

template <SortMethod method>
void testHistogram(uint64_t size = 10000) {
  uint64_t uniqueSize = std::min(100UL, size);
  using Url = Bytes<16>;
  using InputVec = StdVector<Url>;
  using OutputVec = StdVector<HistEntry<Url>>;
  InputVec inputs(size);
  OutputVec outputs(size);
  typename InputVec::Writer inputWriter(inputs.begin(), inputs.end());
  std::vector<Url> uniqueUrls(uniqueSize);
  for (uint64_t i = 0; i < uniqueSize; ++i) {
    uniqueUrls[i].SetRand();
  }
  std::sort(uniqueUrls.begin(), uniqueUrls.end());
  std::vector<uint64_t> counts(uniqueSize);
  for (uint64_t i = 0; i < size; ++i) {
    uint64_t idx = UniformRandom(uniqueSize - 1);
    inputWriter.write(uniqueUrls[idx]);
    ++counts[idx];
  }
  typename InputVec::Reader inputReader(inputs.begin(), inputs.end());
  typename OutputVec::Writer outputWriter(outputs.begin(), outputs.end());
  histogram<method>(inputReader, outputWriter);
  typename OutputVec::Reader outputReader(outputs.begin(), outputs.end());
  for (uint64_t i = 0; i < uniqueSize; ++i) {
    HistEntry<Url> entry = outputReader.read();
    EXPECT_EQ(entry.key, uniqueUrls[i]);
    EXPECT_EQ(entry.count, counts[i]);
  }
}

template <SortMethod method>
void testHistogramPerf(uint64_t size) {
  using Url = Bytes<256>;
  using HistEntry_ = Apps::HistEntry<Url>;
  EM::VirtualVector::VirtualReader<Url> inputReader(
      size, [&](uint64_t i) { return Url{}; });

  EM::VirtualVector::VirtualWriter<HistEntry_> outputWriter(
      size, [&](uint64_t i, const HistEntry_& entry) {});
  Apps::histogram<method>(inputReader, outputWriter);
}

TEST(TestApps, HistogramCorrectness) {
  testHistogram<KWAYBUTTERFLYOSORT>(10000);
  testHistogram<BITONICSORT>(10000);
}

TEST(TestApps, HistogramPerf) {
  testHistogramPerf<KWAYBUTTERFLYOSORT>(10000000);
  testHistogramPerf<BITONICSORT>(10000000);
}

template <SortMethod method>
void testOramInit(uint64_t size = 65536) {
  static constexpr uint64_t Z = 2;
  EM::VirtualVector::VirtualReader<ORAMEntry> inputReader(
      size, [&](uint64_t i) {
        ORAMEntry entry;
        entry.uid = i;
        entry.pos = UniformRandom(size - 1);
        return entry;
      });
  StdVector<ORAMEntry> output((size * 2 - 1) * Z);
  typename StdVector<ORAMEntry>::Writer outputWriter(output.begin(),
                                                     output.end());
  ORAMInit<method, Z>(inputReader, outputWriter);
  uint64_t idx = 0;
  for (int level = GetLogBaseTwo(size); level >= 0; --level) {
    int botLevel = GetLogBaseTwo(size) - level;
    for (int offset = 0; offset < (1 << level); ++offset) {
      for (int i = 0; i < Z; ++i, ++idx) {
        const ORAMEntry& entry = output[idx];
        if (entry.uid != (uint64_t)-1) {
          if (entry.pos >> botLevel != offset) {
            std::cerr << "level: " << level << " offset: " << offset
                      << " entry.pos: " << entry.pos << std::endl;
          }
          EXPECT_EQ(entry.pos >> botLevel, offset);
        }
      }
    }
  }
}

TEST(TestApps, OramInit) {
  testOramInit<KWAYBUTTERFLYOSORT>(65536);
  testOramInit<BITONICSORT>(65536);
}