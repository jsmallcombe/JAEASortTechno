#ifndef JAEASortThreadedHistograms
#define JAEASortThreadedHistograms

#include <ROOT/TThreadedObject.hxx>
#include <TDirectory.h>
#include <TH1F.h>
#include <TH2D.h>
#include <TH2F.h>
#include <TH3.h>
#include <TH3F.h>
#include <TString.h>

#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

using ROOT::TThreadedObject;

#define JAEA_THREADED_HISTOGRAM_LIST(X) \
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
    X(TH1F, s3_raw_ring_mult, "s3_raw", "S3 raw ring multiplicity", 11, -0.5, 10.5) \
    X(TH1F, s3_raw_sector_mult, "s3_raw", "S3 raw sector multiplicity", 11, -0.5, 10.5) \
    X(TH2F, s3_raw_ring_energy, "s3_raw", "S3 raw ring energy;Ring;Ring energy", 24, -0.5, 23.5, 1024, 0, 8192) \
    X(TH2F, s3_raw_sector_energy, "s3_raw", "S3 raw sector energy;Sector;Sector energy", 32, -0.5, 31.5, 1024, 0, 8192) \
    X(TH2F, s3_raw_ring_sector, "s3_raw", "S3 raw ring vs sector;Sector;Ring", 32, -0.5, 31.5, 24, -0.5, 23.5) \
    X(TH2F, s3_raw_ring_sector_energy, "s3_raw", "S3 raw ring energy vs sector energy;Ring energy;Sector energy", 512, 0, 8192, 512, 0, 8192) \
    X(TH3I, s3_raw_sector_vs_sector_ring_energy, "s3_raw", "S3 raw sector vs sector energy vs ring energy;Sector;Sector energy;Ring energy", 32, -0.5, 31.5, 52, 0, 8192, 52, 0, 8192) \
    X(TH3I, s3_raw_ring_vs_sector_ring_energy, "s3_raw", "S3 raw ring vs sector energy vs ring energy;Ring;Sector energy;Ring energy", 24, -0.5, 23.5, 52, 0, 8192, 52, 0, 8192) \
    X(TH1F, s3_raw_ring_sector_dt, "s3_raw", "S3 raw ring-sector time difference;Ring - sector time", 201, -1005, 1005) \
    X(TH2F, s3_raw_ring_dt, "s3_raw", "S3 raw ring vs ring-sector time difference;Ring;Ring - sector time", 24, -0.5, 23.5, 201, -1005, 1005) \
    X(TH2F, s3_raw_sector_dt, "s3_raw", "S3 raw sector vs ring-sector time difference;Sector;Ring - sector time", 32, -0.5, 31.5, 201, -1005, 1005) \
    X(TH1F, s3_pixel_mult, "s3_pixels", "S3 built pixel multiplicity", 11, -0.5, 10.5) \
    X(TH2F, s3_pixel_ring_sector, "s3_pixels", "S3 built pixel ring vs sector;Sector;Ring", 32, -0.5, 31.5, 24, -0.5, 23.5) \
    X(TH1F, s3_pixel_energy, "s3_pixels", "S3 built pixel primary energy;Energy", 1024, 0, 8192) \
    X(TH2F, s3_pixel_ring_energy, "s3_pixels", "S3 built pixel ring vs primary energy;Ring;Energy", 24, -0.5, 23.5, 1024, 0, 8192) \
    X(TH2F, s3_pixel_sector_energy, "s3_pixels", "S3 built pixel sector vs primary energy;Sector;Energy", 32, -0.5, 31.5, 1024, 0, 8192) \
    X(TH2F, s3_pixel_ring_sector_energy, "s3_pixels", "S3 built pixel ring energy vs sector energy;Ring energy;Sector energy", 512, 0, 8192, 512, 0, 8192) \
    X(TH1F, s3_pixel_ring_sector_dt, "s3_pixels", "S3 built pixel ring-sector time difference;Ring - sector time", 201, -1005, 1005) \
    X(TH2F, s3_pixel_theta_energy, "s3_pixels", "S3 built pixel theta vs energy;Theta [rad];Energy", 180, 0, 3.14159265358979323846, 1024, 0, 8192) \
    X(TH2F, s3_pixel_position_xy, "s3_pixels", "S3 built pixel position;X;Y", 200, -50, 50, 200, -50, 50) \
    X(TH3F, s3_pixel_position_xyz, "s3_pixels", "S3 built pixel position;X;Y;Z", 120, -60, 60, 120, -60, 60, 120, -60, 60) \
    X(TH2D, sector_ring_energy_double, "correlations", "Sector E vs Ring E", 512, 0, 8192, 512, 0, 8192) \
    X(TH2F, cdte_chan, "gammas", "CdTe Evergy vs Channel;Channel;Energy", 16, -0.5, 15.5, 1024, 0, 8192) \
    X(TH2F, cdte_S3time, "gammas", "CdTe Channel vs S3-CdTe time;Channel;Time", 16, -0.5, 15.5, 201, -1000.5, 1000.5) \
    X(TH2F, cdte_S3time_gate, "gammas", "CdTe Channel vs S3-CdTe time;Channel;Time", 16, -0.5, 15.5, 201, -1000.5, 1000.5) \
    X(TH2F, cdte_S3, "gammas", "CdTe Evergy vs Channel (S3 Gated);Channel;Energy", 16, -0.5, 15.5, 1024, 0, 8192) \
    X(TH2F, hpge_chan, "gammas", "HPGe Evergy vs Channel;Channel;Energy", 16, -0.5, 15.5, 1024, 0, 8192) \
    X(TH2F, hpge_S3time, "gammas", "HPGe Channel vs S3-HPGe time;Channel;Time", 16, -0.5, 15.5, 201, -1000.5, 1000.5) \
    X(TH2F, hpge_S3time_gate, "gammas", "HPGe Channel vs S3-HPGe time;Channel;Time", 16, -0.5, 15.5, 201, -1000.5, 1000.5) \
    X(TH2F, hpge_S3, "gammas", "HPGe Evergy vs Channel (S3 Gated);Channel;Energy", 16, -0.5, 15.5, 1024, 0, 8192) 

struct HistogramRefs {
    #define JAEA_DECLARE_REF(Type, Name, Directory, ...) Type* Name;
        JAEA_THREADED_HISTOGRAM_LIST(JAEA_DECLARE_REF)
    #undef JAEA_DECLARE_REF

    TH2F* ESumPart[4];
    TH2F* ModulesRaw[4];
};


class ThreadedHistogramSet {
public:
    #define JAEA_DECLARE_THREADED_HIST(Type, Name, Directory, ...)  TThreadedObject<Type> Name{#Name, __VA_ARGS__};
    JAEA_THREADED_HISTOGRAM_LIST(JAEA_DECLARE_THREADED_HIST)
    #undef JAEA_DECLARE_THREADED_HIST

    std::unique_ptr<TThreadedObject<TH2F>> ESumPart[4];
    std::unique_ptr<TThreadedObject<TH2F>> ModulesRaw[4];

    ThreadedHistogramSet()
    {
        #define JAEA_REGISTER_THREADED_HIST(Type, Name, Directory, ...) Register(Name, Directory);
        JAEA_THREADED_HISTOGRAM_LIST(JAEA_REGISTER_THREADED_HIST)
        #undef JAEA_REGISTER_THREADED_HIST

        for (int i = 0; i < 4; ++i) {
            ESumPart[i].reset(new TThreadedObject<TH2F>(Form("ESumPart_%d", i),Form("E%d vs Esum;Esum;E%d", i, i),
                500, 0, 2000, 500, 0, 2000));
            Register(*ESumPart[i], "grouped");
            
            ModulesRaw[i].reset(new TThreadedObject<TH2F>(Form("Module%d_ADC", i),Form("Module %d channel vs ADC", i),32, 0, 32, 1024, 0, 8192));
            Register(*ModulesRaw[i], "modules");
        }
    }

    HistogramRefs ResolveHistogramRefs()
    {
        HistogramRefs refs;

        #define JAEA_RESOLVE_REF(Type, Name, Directory, ...) refs.Name = Name.Get().get();
        JAEA_THREADED_HISTOGRAM_LIST(JAEA_RESOLVE_REF)
        #undef JAEA_RESOLVE_REF

        for (int i = 0; i < 4; ++i) {
            refs.ESumPart[i] = ESumPart[i]->Get().get();
            refs.ModulesRaw[i] = ModulesRaw[i]->Get().get();
        }

        return refs;
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

#undef JAEA_THREADED_HISTOGRAM_LIST

#endif
