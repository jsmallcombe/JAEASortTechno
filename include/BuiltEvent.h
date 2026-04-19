#ifndef JAEASortBuiltEvent
#define JAEASortBuiltEvent

#include <Rtypes.h>
#include <cstring>
#include <vector>

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

// =========================
// BuiltEvent
// =========================
struct BuiltEvent {
    std::vector<UShort_t> Ts;
    std::vector<UShort_t> Mod;
    std::vector<UShort_t> Ch;
    std::vector<UShort_t> Adc;

    void Clear()
    {
        Ts.clear();
        Mod.clear();
        Ch.clear();
        Adc.clear();
    }

    bool Empty() const
    {
        return Ts.empty();
    }

    size_t Size() const
    {
        return Ts.size();
    }

    void StartEvent(const Event& ev)
    {
        Clear();
        Ts.push_back(0);
        Mod.push_back(ev.mod);
        Ch.push_back(ev.ch);
        Adc.push_back(ev.adc);
    }

    void AppendHit(const Event& ev, Long64_t firstTs)
    {
        Ts.push_back(SafeTsDiff(ev.ts, firstTs));
        Mod.push_back(ev.mod);
        Ch.push_back(ev.ch);
        Adc.push_back(ev.adc);
    }
};

struct BuiltEventView {
    const std::vector<UShort_t>& Ts;
    const std::vector<UShort_t>& Mod;
    const std::vector<UShort_t>& Ch;
    const std::vector<UShort_t>& Adc;

    size_t Size() const
    {
        return Ts.size();
    }
};

inline BuiltEventView MakeBuiltEventView(const BuiltEvent& event)
{
    return BuiltEventView{event.Ts, event.Mod, event.Ch, event.Adc};
}

class BuiltEventChunkBuffer {
public:
    std::vector<BuiltEvent> Events;
    size_t ApproxBytes = 0;

    void Branch(const char* name, std::vector<UShort_t>* branch)
    {
        if (std::strcmp(name, "Ts") == 0) {
            TsBranch = branch;
        } else if (std::strcmp(name, "Mod") == 0) {
            ModBranch = branch;
        } else if (std::strcmp(name, "Ch") == 0) {
            ChBranch = branch;
        } else if (std::strcmp(name, "Adc") == 0) {
            AdcBranch = branch;
        }
    }

    void Fill()
    {
        if (!TsBranch || !ModBranch || !ChBranch || !AdcBranch) {
            return;
        }

        BuiltEvent& event = Events.emplace_back();
        event.Ts = *TsBranch;
        event.Mod = *ModBranch;
        event.Ch = *ChBranch;
        event.Adc = *AdcBranch;

        ApproxBytes += (event.Ts.size() + event.Mod.size() + event.Ch.size() + event.Adc.size()) * sizeof(UShort_t);
    }

    void FillMove()
    {
        if (!TsBranch || !ModBranch || !ChBranch || !AdcBranch) {
            return;
        }

        BuiltEvent& event = Events.emplace_back();
        event.Ts.swap(*TsBranch);
        event.Mod.swap(*ModBranch);
        event.Ch.swap(*ChBranch);
        event.Adc.swap(*AdcBranch);

        ApproxBytes += (event.Ts.size() + event.Mod.size() + event.Ch.size() + event.Adc.size()) * sizeof(UShort_t);
    }

    bool Empty() const
    {
        return Events.empty();
    }

    size_t Size() const
    {
        return Events.size();
    }

    size_t GetApproxBytes() const
    {
        return ApproxBytes;
    }

private:
    std::vector<UShort_t>* TsBranch = nullptr;
    std::vector<UShort_t>* ModBranch = nullptr;
    std::vector<UShort_t>* ChBranch = nullptr;
    std::vector<UShort_t>* AdcBranch = nullptr;
};


// Create one chunk buffer that mirrors the same branch-style interface as
// the output tree, so the builder can write tree and histogram handoff data
// through the same event vectors with minimal extra logic.
inline BuiltEventChunkBuffer* CreateBuiltEventChunkBuffer(BuiltEvent& event)
{
    BuiltEventChunkBuffer* chunk = new BuiltEventChunkBuffer();
    chunk->Branch("Ts", &event.Ts);
    chunk->Branch("Mod", &event.Mod);
    chunk->Branch("Ch", &event.Ch);
    chunk->Branch("Adc", &event.Adc);
    return chunk;
}


#endif
