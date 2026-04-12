#ifndef JAEASortBuiltEvent
#define JAEASortBuiltEvent

#include <Rtypes.h>
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

#endif
