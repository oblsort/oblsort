#pragma once
#include "external_memory/algorithm/kway_butterfly_sort.hpp"
#include "external_memory/virtualvector.hpp"


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
    template <class Reader, class Writer, const SortMethod method>
    void histogram(Reader& reader, Writer& writer) {
        uint64_t size = reader.size();
        using T = typename Reader::value_type;
        
        static_assert(method == KWAYBUTTERFLYOSORT || method == BITONICSORT, "Invalid method for histogram");
        if constexpr (method == KWAYBUTTERFLYOSORT) {
            uint64_t dupCounter = 0;
            T prev;
            if (DEFAULT_HEAP_SIZE < sizeof(HistEntry) * size * 2) { // external memory
                using IntermediateVec = EM::NonCachedVector::Vector<HistEntry, (1UL << 16)>;
                IntermediateVec hist(size);
                typename IntermediateVec::Writer histWriter(hist.begin(), hist.end());

                EM::VirtualVector::VirtualWriter<T> vWriter(size, [&](uint64_t idx, const T& ele) {
                    if (idx == 0) {
                        prev = ele;
                        return;
                    }
                    bool isDup = prev == ele;
                    ++dupCounter;
                    uint64_t toWriteFreq = dupCounter;
                    obliMove(isDup, toWriteFreq, 0); // dummy element
                    histWriter.write(HistEntry{prev, toWriteFreq});
                    obliMove(!isDup, dupCounter, 0);
                });
                
                KWayButterflySort(reader, vWriter, DEFAULT_HEAP_SIZE);
                histWriter.write(HistEntry{prev, ++dupCounter});
                typename IntermediateVec::Reader histReader(hist.begin(), hist.end());
                KWayButterflySort(histReader, writer, DEFAULT_HEAP_SIZE);
                
            }
            
            

        } else if constexpr (method == BITONICSORT) {
            BitonicSort(begin, end);
        }

    }
}