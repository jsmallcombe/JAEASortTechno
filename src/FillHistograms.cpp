#include <FillHistograms.h>

#include <ROOT/TTreeProcessorMT.hxx>
#include <IO.h>
#include <Detectors.h>
#include <TFile.h>
#include <TMemFile.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include <atomic>
#include <chrono>
#include <thread>

void FillHistograms(HistogramRefs& H, const BuiltEventView& event);

namespace {
struct DetHitScratch {
    std::vector<DetHit> hpge;
    std::vector<DetHit> laBr;
    std::vector<DetHit> siDeltaE;
    std::vector<DetHit> si;
    std::vector<DetHit> siDeltaE_B;
    std::vector<DetHit> si_B;
    std::vector<DetHit> solar;
    std::vector<DetHit> dice;
    std::vector<DetHit> cdte;
};

DetHitScratch& DetHitScratchBuffer()
{
    thread_local DetHitScratch scratch;
    return scratch;
}

DetHitScratch& BuildDetHitCategories(const BuiltEventView& event)
{
    DetHitScratch& scratch = DetHitScratchBuffer();

    scratch.hpge.clear();
    scratch.laBr.clear();
    scratch.siDeltaE.clear();
    scratch.si.clear();
    scratch.siDeltaE_B.clear();
    scratch.si_B.clear();
    scratch.solar.clear();
    scratch.dice.clear();
    scratch.cdte.clear();

    scratch.hpge.reserve(event.Size());
    scratch.laBr.reserve(event.Size());
    scratch.siDeltaE.reserve(event.Size());
    scratch.si.reserve(event.Size());
    scratch.siDeltaE_B.reserve(event.Size());
    scratch.si_B.reserve(event.Size());
    scratch.solar.reserve(event.Size());
    scratch.dice.reserve(event.Size());
    scratch.cdte.reserve(event.Size());

    for (size_t i = 0; i < event.Size(); ++i) {
        const UShort_t mod = event.Mod[i];
        const UShort_t ch = event.Ch[i];

        switch (DetHit::GetDetType(mod, ch)) {
            case DetHit::HPGe:
                scratch.hpge.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::LaBr:
                scratch.laBr.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::SiDeltaE:
                scratch.siDeltaE.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::Si:
                scratch.si.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::SiDeltaE_B:
                scratch.siDeltaE_B.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::Si_B:
                scratch.si_B.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::Solar:
                scratch.solar.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::Dice:
                scratch.dice.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::CdTe:
                scratch.cdte.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            default:
                break;
        }
    }

    return scratch;
}

struct HistogramStageTimers {
    std::chrono::steady_clock::duration setup{};
    std::chrono::steady_clock::duration fill{};
    std::chrono::steady_clock::duration write{};
    std::chrono::steady_clock::duration wait{};
    size_t chunksProcessed = 0;

    void Print(const char* label,
               size_t eventsProcessed) const
    {
        using namespace std::chrono;
        const double setupSec = duration<double>(setup).count();
        const double fillSec = duration<double>(fill).count();
        const double writeSec = duration<double>(write).count();
        const double waitSec = duration<double>(wait).count();
        const double totalSec = setupSec + fillSec + writeSec;

        std::cout << "\n[" << label << " stage timings]\n"
                  << "  setup/init       : " << setupSec << " s\n"
                  << "  fill             : " << fillSec << " s\n"
                  << "  write            : " << writeSec << " s\n"
                  << "  queue wait       : " << waitSec << " s\n"
                  << "  subtotal         : " << totalSec << " s\n"
                  << "  events processed : " << eventsProcessed << "\n"
                  << "  chunks processed : " << chunksProcessed << "\n";
    }
};

TTree* GetEventTreeFromMemFile(TMemFile* memFile)
{
    if (!memFile) {
        return nullptr;
    }
    return dynamic_cast<TTree*>(memFile->Get("EventTree"));
}

size_t FillHistogramsFromEventTreeSequential(TTree* tree, HistogramRefs& refs)
{
    if (!tree) {
        return 0;
    }

    std::vector<UShort_t>* ts = 0;
    std::vector<UShort_t>* mod = 0;
    std::vector<UShort_t>* ch = 0;
    std::vector<UShort_t>* adc = 0;

    tree->SetBranchAddress("Ts", &ts);
    tree->SetBranchAddress("Mod", &mod);
    tree->SetBranchAddress("Ch", &ch);
    tree->SetBranchAddress("Adc", &adc);

    const Long64_t nentries = tree->GetEntries();
    size_t processed = 0;

    for (Long64_t entry = 0; entry < nentries; ++entry) {
        tree->GetEntry(entry);
        const BuiltEventView event{*ts, *mod, *ch, *adc};
        FillHistograms(refs, event);
        ++processed;
    }

    return processed;
}

}

void FillHistograms(HistogramRefs& H, const BuiltEventView& event)
{
    // DetHitScratch& detHits = BuildDetHitCategories(event);
    // auto& hpge = detHits.hpge;
    // auto& si = detHits.si;

    for (size_t i = 0; i < event.Size(); ++i) {
        H.mod_ch_adc_ts->Fill(event.Mod[i], event.Ch[i], event.Adc[i]);

        if (event.Mod[i] == 2) {
            H.mod1_ch_adc->Fill(event.Ch[i], event.Adc[i]);
        }
        if (event.Mod[i] == 1) {
            H.mod2_ch_adc->Fill(event.Ch[i], event.Adc[i]);
            for (size_t j = 0; j < event.Size(); ++j) {
                if (event.Mod[j] == 2) {
                    const double dT = static_cast<double>(event.Ts[i]) - static_cast<double>(event.Ts[j]);
                    H.siall->Fill(event.Ch[j], event.Ch[i]);
                    H.ring_sector_E->Fill(event.Adc[j], event.Adc[i]);
                    H.sector_ring_energy_double->Fill(static_cast<double>(event.Adc[j]), static_cast<double>(event.Adc[i]));
                    if (event.Ch[i] != 11 && event.Ch[i] != 16 && event.Ch[i] != 17 && event.Ch[i] != 18) {
                        H.ring_sector_E_reduced->Fill(event.Adc[j], event.Adc[i]);
                        H.sidt->Fill(dT);
                    }
                    if (dT > -100.0 && dT < 100.0) {
                        H.pmpt_ring_sector_E->Fill(event.Adc[j], event.Adc[i]);
                        if (event.Ch[i] != 11 && event.Ch[i] != 16 && event.Ch[i] != 17 && event.Ch[i] != 18) {
                            H.pmpt_ring_sector_E_reduced->Fill(event.Adc[j], event.Adc[i]);
                        }

                        if (event.Adc[i] != 0) {
                            H.hSectE_divRingE->Fill(static_cast<double>(event.Adc[j]) / static_cast<double>(event.Adc[i]));
                        }

                        if (event.Ch[i] < 4) {
                            H.ESumPart[event.Ch[i]]->Fill(event.Adc[i] + event.Adc[j], event.Adc[i]);
                        }

                        if (event.Adc[i] > 120 && event.Adc[j] > 120) {
                            for (size_t k = 0; k < event.Size(); ++k) {
                                if (event.Mod[k] == 3 && event.Ch[k] > 7) {
                                    const double dT_sect_cdte = static_cast<double>(event.Ts[j]) - static_cast<double>(event.Ts[k]);
                                    H.hSect_CdTe_dT->Fill(dT_sect_cdte);
                                    H.hSect_CdTe_dT_ADC->Fill(dT_sect_cdte, event.Adc[k]);
                                }
                                if (event.Mod[k] == 4) {
                                    const double dT_sect_hpge = static_cast<double>(event.Ts[j]) - static_cast<double>(event.Ts[k]);
                                    H.hSect_HPGe_dT->Fill(dT_sect_hpge);
                                    H.hSect_HPGe_dT_ADC->Fill(dT_sect_hpge, event.Adc[k]);
                                }
                            }
                        }
                    }
                }
                if (event.Mod[j] == 1 && j != i) {
                    H.hRingRing->Fill(event.Ch[i], event.Ch[j]);
                }
            }
        }
        if (event.Mod[i] == 2) {
            for (size_t j = 0; j < event.Size(); ++j) {
                if (event.Mod[j] == 2 && i != j) {
                    H.hSectSect->Fill(event.Ch[i], event.Ch[j]);
                }
            }
        }
        if (event.Mod[i] == 3) {
            H.mod3_ch_adc->Fill(event.Ch[i], event.Adc[i]);
        }
        if (event.Mod[i] == 4) {
            H.mod4_ch_adc->Fill(event.Ch[i], event.Adc[i]);
        }
    }

}

void FillHistograms(ThreadedHistogramSet& histograms, const BuiltEventView& event)
{
    HistogramRefs refs = histograms.ResolveHistogramRefs();
    FillHistograms(refs, event);
}

void FillHistogramsFromEventTree(TTree* tree,
                                 ThreadedHistogramSet& histograms,
                                 unsigned int nthreads)
{
    if (!tree) {
        return;
    }

    HistogramStageTimers timers;
    std::atomic<size_t> eventsProcessed{0};

    if (nthreads <= 1) {
        std::vector<UShort_t>* ts = 0;
        std::vector<UShort_t>* mod = 0;
        std::vector<UShort_t>* ch = 0;
        std::vector<UShort_t>* adc = 0;

        tree->SetBranchAddress("Ts", &ts);
        tree->SetBranchAddress("Mod", &mod);
        tree->SetBranchAddress("Ch", &ch);
        tree->SetBranchAddress("Adc", &adc);

        const auto setupStart = std::chrono::steady_clock::now();
        HistogramRefs refs = histograms.ResolveHistogramRefs();
        timers.setup += std::chrono::steady_clock::now() - setupStart;
        Long64_t nentries = tree->GetEntries();

        const auto fillStart = std::chrono::steady_clock::now();
        for (Long64_t entry = 0; entry < nentries; ++entry) {
            tree->GetEntry(entry);
            const BuiltEventView event{*ts, *mod, *ch, *adc};
            FillHistograms(refs, event);
            ++eventsProcessed;
        }
        timers.fill += std::chrono::steady_clock::now() - fillStart;
        timers.Print("FillHistogramsFromEventTree", eventsProcessed.load());
        return;
    }

    const auto setupStart = std::chrono::steady_clock::now();
    ROOT::EnableImplicitMT(nthreads);
    ROOT::TTreeProcessorMT processor(*tree, nthreads);
    timers.setup += std::chrono::steady_clock::now() - setupStart;

    const auto fillStart = std::chrono::steady_clock::now();
    processor.Process([&histograms, &eventsProcessed](TTreeReader& reader) {
        TTreeReaderValue<std::vector<UShort_t>> ts(reader, "Ts");
        TTreeReaderValue<std::vector<UShort_t>> mod(reader, "Mod");
        TTreeReaderValue<std::vector<UShort_t>> ch(reader, "Ch");
        TTreeReaderValue<std::vector<UShort_t>> adc(reader, "Adc");
        HistogramRefs refs = histograms.ResolveHistogramRefs();

        while (reader.Next()) {
            const BuiltEventView event{*ts, *mod, *ch, *adc};
            FillHistograms(refs, event);
            ++eventsProcessed;
        }
    });
    timers.fill += std::chrono::steady_clock::now() - fillStart;
    timers.Print("FillHistogramsFromEventTree", eventsProcessed.load());
}

void FillHistogramsFromBuiltEventQueue(ThreadSafeQueue<BuiltEvent>& queue,
                                       ThreadedHistogramSet& histograms,
                                       unsigned int nthreads)
{
    HistogramStageTimers timers;
    std::atomic<size_t> eventsProcessed{0};

    const auto setupStart = std::chrono::steady_clock::now();
    ROOT::EnableThreadSafety();
    histograms.ResolveHistogramRefs();
    timers.setup += std::chrono::steady_clock::now() - setupStart;

    unsigned int workerCount = nthreads;
    if (workerCount == 0) {
        workerCount = std::thread::hardware_concurrency();
    }
    if (workerCount == 0) {
        workerCount = 1;
    }

    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    const auto fillStart = std::chrono::steady_clock::now();
    for (unsigned int i = 0; i < workerCount; ++i) {
        workers.push_back(std::thread([&queue, &histograms, &eventsProcessed]() {
            BuiltEvent event;
            HistogramRefs refs = histograms.ResolveHistogramRefs();
            while (queue.pop(event)) {
                FillHistograms(refs, MakeBuiltEventView(event));
                ++eventsProcessed;
                event.Clear();
            }
        }));
    }

    for (size_t i = 0; i < workers.size(); ++i) {
        workers[i].join();
    }
    timers.fill += std::chrono::steady_clock::now() - fillStart;
    timers.Print("FillHistogramsFromBuiltEventQueue", eventsProcessed.load());
}

void FillHistogramsFromEventTreeQueue(ThreadSafeQueue<TMemFile*>& queue,
                                      ThreadedHistogramSet& histograms,
                                      unsigned int nthreads)
{
    HistogramStageTimers timers;
    std::atomic<size_t> eventsProcessed{0};
    std::atomic<size_t> chunksProcessed{0};
    std::atomic<long long> queueWaitNs{0};
    std::atomic<long long> fillNs{0};

    const auto setupStart = std::chrono::steady_clock::now();
    ROOT::EnableThreadSafety();
    histograms.ResolveHistogramRefs();
    timers.setup += std::chrono::steady_clock::now() - setupStart;

    unsigned int workerCount = nthreads;
    if (workerCount == 0) {
        workerCount = std::thread::hardware_concurrency();
    }
    if (workerCount == 0) {
        workerCount = 1;
    }

    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    for (unsigned int i = 0; i < workerCount; ++i) {
        workers.push_back(std::thread([&queue, &histograms, &eventsProcessed, &chunksProcessed, &queueWaitNs, &fillNs]() {
            HistogramRefs refs = histograms.ResolveHistogramRefs();
            TMemFile* memFile = nullptr;

            while (true) {
                const auto waitStart = std::chrono::steady_clock::now();
                const bool ok = queue.pop(memFile);
                queueWaitNs.fetch_add(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                          std::chrono::steady_clock::now() - waitStart)
                                          .count(),
                                      std::memory_order_relaxed);
                if (!ok) {
                    break;
                }

                const auto fillStart = std::chrono::steady_clock::now();
                TTree* tree = GetEventTreeFromMemFile(memFile);
                eventsProcessed += FillHistogramsFromEventTreeSequential(tree, refs);
                fillNs.fetch_add(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                     std::chrono::steady_clock::now() - fillStart)
                                     .count(),
                                 std::memory_order_relaxed);
                ++chunksProcessed;
                delete memFile;
                memFile = nullptr;
            }
        }));
    }

    for (std::thread& worker : workers) {
        worker.join();
    }
    timers.fill += std::chrono::nanoseconds(fillNs.load());
    timers.wait += std::chrono::nanoseconds(queueWaitNs.load());
    timers.chunksProcessed = chunksProcessed.load();
    timers.Print("FillHistogramsFromEventTreeQueue", eventsProcessed.load());
}

void FillHistogramsFromEventTreeQueueUsingExistingFunction(ThreadSafeQueue<TMemFile*>& queue,
                                                           ThreadedHistogramSet& histograms,
                                                           unsigned int nthreads)
{
    HistogramStageTimers timers;
    std::atomic<size_t> eventsProcessed{0};

    const auto setupStart = std::chrono::steady_clock::now();
    ROOT::EnableThreadSafety();
    timers.setup += std::chrono::steady_clock::now() - setupStart;

    const auto fillStart = std::chrono::steady_clock::now();
    TMemFile* memFile = nullptr;
    bool warnedAboutMemFileMT = false;
    while (queue.pop(memFile)) {
        TTree* tree = GetEventTreeFromMemFile(memFile);
        if (tree) {
            eventsProcessed += static_cast<size_t>(tree->GetEntries());
            unsigned int treeThreads = nthreads;
            if (treeThreads > 1 && dynamic_cast<TMemFile*>(tree->GetCurrentFile()) != nullptr) {
                if (!warnedAboutMemFileMT) {
                    std::cout << "FillHistogramsFromEventTree unchanged path cannot use TTreeProcessorMT "
                              << "on TMemFile chunks; falling back to single-thread chunk reads.\n";
                    warnedAboutMemFileMT = true;
                }
                treeThreads = 1;
            }
            FillHistogramsFromEventTree(tree, histograms, treeThreads);
        }
        delete memFile;
        memFile = nullptr;
    }
    timers.fill += std::chrono::steady_clock::now() - fillStart;
    timers.Print("FillHistogramsFromEventTreeQueueUsingExistingFunction", eventsProcessed.load());
}

bool WriteHistogramFile(ThreadedHistogramSet& histograms,
                        const TString& outfilename,
                        bool overwrite)
{
    if (!TestOutputPath(outfilename, overwrite, "Histogram")) {
        return false;
    }

    TFile* outfile = TFile::Open(outfilename, "RECREATE");
    if (!outfile || outfile->IsZombie()) {
        std::cerr << "Could not create histogram output file " << outfilename << '\n';
        delete outfile;
        return false;
    }

    HistogramStageTimers timers;
    const auto writeStart = std::chrono::steady_clock::now();
    outfile->cd();
    histograms.WriteAll(outfile);
    outfile->Write("", TObject::kOverwrite);
    timers.write += std::chrono::steady_clock::now() - writeStart;
    outfile->Close();
    delete outfile;
    timers.Print("WriteHistogramFile", 0);
    return true;
}
