#ifndef JAEASortThreadedHistograms
#define JAEASortThreadedHistograms

#include <ROOT/TThreadedObject.hxx>
#include <TDirectory.h>
#include <TH1F.h>
#include <TH2D.h>
#include <TH2F.h>
#include <TH3F.h>
#include <TString.h>

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

using ROOT::TThreadedObject;

class ThreadedHistogramSet {
public:
    TThreadedObject<TH2F> siall{"siall", "rings vs sectors", 32, 0, 32, 32, 0, 32};
    TThreadedObject<TH1F> sidt{"sid", "rings and sectors time differences", 200, -1000, 1000};
    TThreadedObject<TH2F> ring_sector_E{"ring_sector_E", "ring E vs sector E", 512, 0, 8192, 512, 0, 8192};
    TThreadedObject<TH2F> ring_sector_E_reduced{"ring_sector_E_reduced", "ring E vs sector E", 512, 0, 8192, 512, 0, 8192};
    TThreadedObject<TH2F> pmpt_ring_sector_E{"pmpt_ring_sector_E", "pmpt_ring E vs sector E *Tdiff<100", 512, 0, 8192, 512, 0, 8192};
    TThreadedObject<TH2F> pmpt_ring_sector_E_reduced{"pmpt_ring_sector_E_reduced", "pmpt_ring E vs sector E *Tdiff<100", 512, 0, 8192, 512, 0, 8192};
    TThreadedObject<TH1F> hSect_CdTe_dT{"Sect_CdTe_dT", "Sector - CdTe time difference", 400, -2000, 2000};
    TThreadedObject<TH1F> hSect_HPGe_dT{"Sect_HPGe_dT", "Sector - HPGe time difference", 400, -2000, 2000};
    TThreadedObject<TH2F> hSect_CdTe_dT_ADC{"Sect_CdTe_dT_ADC", "Sector - CdTe time difference vs ADC", 400, -2000, 2000, 1024, 0, 8192};
    TThreadedObject<TH2F> hSect_HPGe_dT_ADC{"Sect_HPGe_dT_ADC", "Sector - HPGe time difference vs ADC", 400, -2000, 2000, 1024, 0, 8192};
    TThreadedObject<TH2F> hRingRing{"RingRing", "Ring # vs Ring #", 32, 0, 32, 32, 0, 32};
    TThreadedObject<TH2F> hSectSect{"SectSect", "Sect # vs Sect #", 32, 0, 32, 32, 0, 32};
    TThreadedObject<TH1F> hSectE_divRingE{"SectE_divRingE", "Sector energy divided by ring energy", 1000, 0, 10};
    TThreadedObject<TH2F> mod1_ch_adc{"mod1_ch_adc", "Module 1 channel vs ADC", 32, 0, 32, 1024, 0, 8192};
    TThreadedObject<TH2F> mod2_ch_adc{"mod2_ch_adc", "Module 2 channel vs ADC", 32, 0, 32, 1024, 0, 8192};
    TThreadedObject<TH2F> mod3_ch_adc{"mod3_ch_adc", "Module 3 channel vs ADC", 32, 0, 32, 1024, 0, 8192};
    TThreadedObject<TH2F> mod4_ch_adc{"mod4_ch_adc", "Module 4 channel vs ADC", 32, 0, 32, 1024, 0, 8192};
    TThreadedObject<TH3F> mod_ch_adc_ts{"mod_ch_adc_ts", "Module vs channel vs ADC", 8, 0, 8, 32, 0, 32, 256, 0, 8192};
    TThreadedObject<TH2D> sector_ring_energy_double{"sector_ring_energy_double", "Sector E vs Ring E", 512, 0, 8192, 512, 0, 8192};

    std::unique_ptr<TThreadedObject<TH2F>> ESumPart[4];

    ThreadedHistogramSet()
    {
        Register(siall, "correlations");
        Register(sidt, "timing");
        Register(ring_sector_E, "correlations");
        Register(ring_sector_E_reduced, "correlations");
        Register(pmpt_ring_sector_E, "correlations");
        Register(pmpt_ring_sector_E_reduced, "correlations");
        Register(hSect_CdTe_dT, "timing");
        Register(hSect_HPGe_dT, "timing");
        Register(hSect_CdTe_dT_ADC, "timing");
        Register(hSect_HPGe_dT_ADC, "timing");
        Register(hRingRing, "correlations");
        Register(hSectSect, "correlations");
        Register(hSectE_divRingE, "correlations");
        Register(mod1_ch_adc, "modules");
        Register(mod2_ch_adc, "modules");
        Register(mod3_ch_adc, "modules");
        Register(mod4_ch_adc, "modules");
        Register(mod_ch_adc_ts, "modules");
        Register(sector_ring_energy_double, "correlations");

        for (int i = 0; i < 4; ++i) {
            ESumPart[i].reset(new TThreadedObject<TH2F>(
                Form("ESumPart_%d", i),
                Form("E%d vs Esum;Esum;E%d", i, i),
                500, 0, 2000, 500, 0, 2000));
            Register(*ESumPart[i], "grouped");
        }
    }

    template <typename HistT>
    void Register(TThreadedObject<HistT>& histogram, const TString& directory = "")
    {
        static_assert(std::is_base_of<TH1, HistT>::value, "HistT must derive from TH1");

        // We store one deferred action per histogram in writeList.
        // That action does not run here during registration.
        // It is only saved so that WriteAll() can execute it later.
        //
        // This stores a deferred call equivalent to:
        //     MergeAndWrite(&histogram);
        //
        // WriteAll() later loops over writeList and executes each stored call.
        // At that point the histogram's thread-local copies are merged and the
        // merged histogram is written into the currently selected directory.
        writeList.push_back(std::bind(&ThreadedHistogramSet::MergeAndWrite<HistT>, &histogram));
        dirlist.push_back(directory);
    }

    void WriteAll(TDirectory* outputDirectory = nullptr)
    {
        TDirectory* previousDirectory = gDirectory;
        if (outputDirectory) {
            outputDirectory->cd();
        }

        TDirectory* baseDirectory = outputDirectory ? outputDirectory : gDirectory;

        for (size_t i = 0; i < writeList.size(); ++i) {
            if (baseDirectory && dirlist[i].Length() > 0) {
                TDirectory* subdir = baseDirectory->GetDirectory(dirlist[i]);
                if (!subdir) {
                    subdir = baseDirectory->mkdir(dirlist[i]);
                }
                subdir->cd();
            } else if (baseDirectory) {
                baseDirectory->cd();
            }

            writeList[i]();
        }

        if (previousDirectory) {
            previousDirectory->cd();
        }
    }

private:
    std::vector<std::function<void()>> writeList;
    std::vector<TString> dirlist;

    template <typename HistT>
    static void MergeAndWrite(TThreadedObject<HistT>* histogram)
    {
        std::unique_ptr<HistT> merged = histogram->SnapshotMerge();
        if (merged) {
            merged->Write();
        }
    }
};

#endif
