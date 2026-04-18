#ifndef JAEASortGlobals
#define JAEASortGlobals

#include <cstddef>  
#include <Rtypes.h>
#include <TRandom3.h>

#include <functional>
#include <thread>

// Event Building
constexpr Long64_t gTS_TOLERANCE = 100000;// Maximum allowed timestamp gap for disordered .bin files
constexpr Long64_t gTS_Diff = 2000; // Maximum allowed timestamp difference for event building (ns)
constexpr size_t gBinChunkDefaultSize=100; // Events read per digitiser
constexpr size_t gBuildBuffDefaultSize=2'000'000; // Nominal size of the event building buffer
constexpr size_t gBuildRefillDivisor=10; // Franction of gBuildBuffDefaultSize to be consumed before refill

// .bin -> Histogram buffer (not used for tree->histogram)
constexpr size_t gThreadQueueBuiltEvents=64'000'000;// Maximum size for built-events waiting for histogram building
constexpr size_t gHistChunkDefaultEvents=100'000;// Split level of built-events for histogram workers

// Colour codes for monitor display
#define CLR_RESET "\033[0m"
#define CLR_GREEN "\033[32m"
#define CLR_YELLOW "\033[33m"
#define CLR_RED "\033[31m"
#define CLR_CYAN "\033[36m"

// =========================
// Event structure
// =========================
struct Event {
    Long64_t ts = -1;
    UShort_t mod = 0;
    UShort_t ch = 0;
    UShort_t adc = 0;

    // Future expansion
//     UShort_t neg = 0;
//     UShort_t unit = 0;
//     UShort_t wavFlag = 0;
//     std::vector<UShort_t> waveform;
    
    bool operator<(const Event& other) const {
        return ts < other.ts;
    }
};

// =========================
// Safe timestamp diff
// =========================
#include <limits>
#include <iostream>
inline UShort_t SafeTsDiff(Long64_t ts, Long64_t firstTs)
{
    Long64_t diff = ts - firstTs;
    
    if (diff < 0 || diff > std::numeric_limits<UShort_t>::max()) {
        std::cout << "[TS ERROR] diff=" << diff << std::endl;
        
        if (diff < 0) diff = 0;
        if (diff > std::numeric_limits<UShort_t>::max())
            diff = std::numeric_limits<UShort_t>::max();
    }
    
    return (UShort_t)diff;
}

inline TRandom3& gThRand()
{
    thread_local TRandom3 rng(static_cast<UInt_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    return rng;
}

#endif
