#ifndef JAEASortBuiltEvent
#define JAEASortBuiltEvent

#include <Rtypes.h>
#include <cstring>
#include <vector>

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

#endif
