#ifndef JAEASortThreadedHistogramsBasic
#define JAEASortThreadedHistogramsBasic

#include <ThreadedHistogramList.h>

#include <TH1F.h>
#include <TH2D.h>
#include <TH2F.h>

#include <memory>

#define JAEA_THREADED_HISTOGRAM_LIST_BASIC(X) \
    X(TH2F, siall, "correlations", "rings vs sectors", 32, 0, 32, 32, 0, 32) \
    X(TH1F, sidt, "timing", "rings and sectors time differences;#Deltat", 201, -1005, 1005) \
    X(TH2F, ring_sector_E, "correlations", "ring E vs sector E", 512, 0, 8192, 512, 0, 8192) \
    X(TH2F, ring_sector_E_reduced, "correlations", "ring E vs sector E", 512, 0, 8192, 512, 0, 8192) \
    X(TH2F, pmpt_ring_sector_E, "correlations", "pmpt_ring E vs sector E *Tdiff<100", 512, 0, 8192, 512, 0, 8192) \
    X(TH2F, pmpt_ring_sector_E_reduced, "correlations", "pmpt_ring E vs sector E *Tdiff<100", 512, 0, 8192, 512, 0, 8192) \
    X(TH1F, hSect_CdTe_dT, "timing", "Sector - CdTe time difference;#Deltat", 400, -2000, 2000) \
    X(TH1F, hSect_HPGe_dT, "timing", "Sector - HPGe time difference;#Deltat", 400, -2000, 2000) \
    X(TH2F, hSect_CdTe_dT_ADC, "timing", "Sector - CdTe time difference vs ADC;#Deltat;ADC", 400, -2000, 2000, 1024, 0, 8192) \
    X(TH2F, hSect_HPGe_dT_ADC, "timing", "Sector - HPGe time difference vs ADC;#Deltat;ADC", 400, -2000, 2000, 1024, 0, 8192) \
    X(TH2F, hRingRing, "correlations", "Ring # vs Ring #", 32, 0, 32, 32, 0, 32) \
    X(TH2F, hSectSect, "correlations", "Sect # vs Sect #", 32, 0, 32, 32, 0, 32) \
    X(TH1F, hSectE_divRingE, "correlations", "Sector energy divided by ring energy", 1000, 0, 10) \
    X(TH2D, sector_ring_energy_double, "correlations", "Sector E vs Ring E", 512, 0, 8192, 512, 0, 8192)

struct HistogramRefsBasic {
    #define JAEA_DECLARE_REF_BASIC(Type, Name, Directory, ...) Type* Name;
        JAEA_THREADED_HISTOGRAM_LIST_BASIC(JAEA_DECLARE_REF_BASIC)
    #undef JAEA_DECLARE_REF_BASIC

    TH2F* ESumPart[4];
    TH2F* ModulesRaw[4];
};

class ThreadedHistogramSetBasic : public ThreadedHistogramList {
public:
    #define JAEA_DECLARE_THREADED_HIST_BASIC(Type, Name, Directory, ...) TThreadedObject<Type> Name{#Name, __VA_ARGS__};
    JAEA_THREADED_HISTOGRAM_LIST_BASIC(JAEA_DECLARE_THREADED_HIST_BASIC)
    #undef JAEA_DECLARE_THREADED_HIST_BASIC

    std::unique_ptr<TThreadedObject<TH2F>> ESumPart[4];
    std::unique_ptr<TThreadedObject<TH2F>> ModulesRaw[4];

    ThreadedHistogramSetBasic()
    {
        #define JAEA_REGISTER_THREADED_HIST_BASIC(Type, Name, Directory, ...) Register(Name, Directory);
        JAEA_THREADED_HISTOGRAM_LIST_BASIC(JAEA_REGISTER_THREADED_HIST_BASIC)
        #undef JAEA_REGISTER_THREADED_HIST_BASIC

        for (int i = 0; i < 4; ++i) {
            ESumPart[i].reset(new TThreadedObject<TH2F>(Form("ESumPart_%d", i),
                                                        Form("E%d vs Esum;Esum;E%d", i, i),
                                                        500, 0, 2000, 500, 0, 2000));
            Register(*ESumPart[i], "grouped");

            ModulesRaw[i].reset(new TThreadedObject<TH2F>(Form("Module%d_ADC", i),
                                                          Form("Module %d channel vs ADC", i),
                                                          32, 0, 32, 1024, 0, 8192));
            Register(*ModulesRaw[i], "modules");
        }
    }

    HistogramRefsBasic ResolveHistogramRefs()
    {
        HistogramRefsBasic refs;

        #define JAEA_RESOLVE_REF_BASIC(Type, Name, Directory, ...) refs.Name = Name.Get().get();
        JAEA_THREADED_HISTOGRAM_LIST_BASIC(JAEA_RESOLVE_REF_BASIC)
        #undef JAEA_RESOLVE_REF_BASIC

        for (int i = 0; i < 4; ++i) {
            refs.ESumPart[i] = ESumPart[i]->Get().get();
            refs.ModulesRaw[i] = ModulesRaw[i]->Get().get();
        }

        return refs;
    }
};

#undef JAEA_THREADED_HISTOGRAM_LIST_BASIC

#endif
