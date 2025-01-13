#include <gtest/gtest.h>

#include "apps/db_join.hpp"
#include "apps/histogram.hpp"
#include "apps/load_balancer.hpp"
#include "apps/oram_init.hpp"
#include "testutils.hpp"
using namespace Apps;

template <SortMethod method>
void testHistogramCorrectness(uint64_t size = 10000) {
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

void compareHistogramPerf(uint64_t size) {
  std::cout << "Histogram size: " << size << std::endl;
  auto start = std::chrono::system_clock::now();

  testHistogramPerf<KWAYBUTTERFLYOSORT>(size);
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = end - start;
  std::cout << "Flex-way Butterfly Sort time (s): " << std::setw(9)
            << diff.count() << std::endl;

  start = std::chrono::system_clock::now();
  testHistogramPerf<BITONICSORT>(size);
  end = std::chrono::system_clock::now();
  diff = end - start;
  std::cout << "Bitonic Sort time (s): " << std::setw(9) << diff.count()
            << std::endl;
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

template <SortMethod method>
void testOramInitPerf(uint64_t size = 65536) {
  static constexpr uint64_t Z = 2;
  EM::VirtualVector::VirtualReader<ORAMEntry> inputReader(
      size, [&](uint64_t i) {
        ORAMEntry entry;
        entry.uid = i;
        entry.pos = UniformRandom(size - 1);
        return entry;
      });
  EM::VirtualVector::VirtualWriter<ORAMEntry> outputWriter(
      (size * 2 - 1) * Z, [&](uint64_t i, const ORAMEntry& entry) {});
  ORAMInit<method, Z>(inputReader, outputWriter);
}

void compareOramInitPerf(uint64_t size) {
  std::cout << "ORAMInit size: " << size << std::endl;
  // start timer
  auto start = std::chrono::system_clock::now();

  testOramInitPerf<KWAYBUTTERFLYOSORT>(size);
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = end - start;
  std::cout << "Flex-way Butterfly Sort time (s): " << std::setw(9)
            << diff.count() << std::endl;

  start = std::chrono::system_clock::now();
  testOramInitPerf<BITONICSORT>(size);
  end = std::chrono::system_clock::now();
  diff = end - start;
  std::cout << "Bitonic Sort time (s): " << std::setw(9) << diff.count()
            << std::endl;
}

template <SortMethod method>
void testLoadBalancer(uint64_t size) {
  using K = Bytes<32>;
  using V = Bytes<32>;
  ParOMap<K, V, uint64_t> testOMap;
  EM::VirtualVector::VirtualReader<std::pair<K, V>> inputReader(
      size, [&](uint64_t i) {
        K key;
        *(uint64_t*)&key = i;
        V value;
        return std::make_pair(key, value);
      });
  testOMap.SetSize(size, 32);
  if constexpr (method == KWAYBUTTERFLYOSORT) {
    testOMap.InitFromReader(inputReader, DEFAULT_HEAP_SIZE);
  } else if constexpr (method == BITONICSORT) {
    testOMap.InitFromReaderBitonic(inputReader);
  }
}

void compareLoadBalancerPerf(uint64_t size) {
  std::cout << "LoadBalancer size: " << size << std::endl;
  auto start = std::chrono::system_clock::now();

  testLoadBalancer<KWAYBUTTERFLYOSORT>(size);
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = end - start;
  std::cout << "Flex-way Butterfly Sort time (s): " << std::setw(9)
            << diff.count() << std::endl;

  start = std::chrono::system_clock::now();
  testLoadBalancer<BITONICSORT>(size);
  end = std::chrono::system_clock::now();
  diff = end - start;
  std::cout << "Bitonic Sort time (s): " << std::setw(9) << diff.count()
            << std::endl;
}
template <SortMethod method>
void testDBJoin(uint64_t size) {
  static constexpr uint64_t payload1Size = 256;
  static constexpr uint64_t payload2Size = 256;
  using DBEntry1 = DBEntry<Bytes<payload1Size>>;
  using DBEntry2 = DBEntry<Bytes<payload2Size>>;
  struct Pair {
    Bytes<payload1Size> first;
    Bytes<payload2Size> second;
  };
  using DBEntry_ = DBEntry<Pair>;
  EM::VirtualVector::VirtualReader<DBEntry1> reader1(size, [&](uint64_t i) {
    DBEntry1 entry;
    entry.id = i * 2;
    return entry;
  });
  EM::VirtualVector::VirtualReader<DBEntry2> reader2(size, [&](uint64_t i) {
    DBEntry2 entry;
    entry.id = i * 3;
    return entry;
  });
  EM::VirtualVector::VirtualWriter<DBEntry_> writer(
      size * 2, [&](uint64_t i, const DBEntry_& entry) {});
  dbJoin<method>(reader1, reader2, writer);
}

void compareDBJoinPerf(uint64_t size) {
  std::cout << "DBJoin size: " << size << std::endl;
  auto start = std::chrono::system_clock::now();

  testDBJoin<KWAYBUTTERFLYOSORT>(size);
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = end - start;
  std::cout << "Flex-way Butterfly Sort time (s): " << std::setw(9)
            << diff.count() << std::endl;

  start = std::chrono::system_clock::now();
  testDBJoin<BITONICSORT>(size);
  end = std::chrono::system_clock::now();
  diff = end - start;
  std::cout << "Bitonic Sort time (s): " << std::setw(9) << diff.count()
            << std::endl;
}

TEST(Apps, HistogramCorrectness) {
  testHistogramCorrectness<KWAYBUTTERFLYOSORT>();
}

TEST(Apps, HistogramPerf) {
  compareHistogramPerf(1UL << 23);
  compareHistogramPerf(1UL << 26);
}

TEST(Apps, OramInitPerf) {
  compareOramInitPerf(1UL << 23);
  compareOramInitPerf(1UL << 26);
}

TEST(Apps, LoadBalancerPerf) {
  compareLoadBalancerPerf(1UL << 23);
  compareLoadBalancerPerf(1UL << 26);
}

TEST(Apps, DBJoinPerf) {
  compareDBJoinPerf(1UL << 23);
  compareDBJoinPerf(1UL << 26);
}