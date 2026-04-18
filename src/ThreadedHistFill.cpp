#include <ThreadedHistFill.h>

#include <ROOT/TTreeProcessorMT.hxx>
#include <IO.h>
#include <TFile.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <memory>
#include <thread>

namespace {

// Sequential inner loop for one completed chunk. Each worker resolves its
// own histogram references once, then reuses them while walking the chunk.
size_t FillHistogramsFromBuiltEventChunkSequential(BuiltEventChunkBuffer& chunk, HistogramRefs& refs)
{
    size_t processed = 0;

    for (const BuiltEvent& event : chunk.Events) {
        FillHistograms(refs, MakeBuiltEventView(event));
        ++processed;
    }

    return processed;
}

struct EventTreeProgressState {
    std::atomic<Long64_t> processed{0};
    std::atomic<bool> done{false};
    Long64_t total = 0;
    std::vector<Long64_t> cumulativeEntries;
    std::vector<TString> inputFiles;
};

// Translate a global entry count into the source file index using the
// cumulative event totals cached by JAEASortIO when the input chain was built.
size_t FindCurrentFileIndex(const EventTreeProgressState& state, Long64_t processed)
{
    if (state.cumulativeEntries.empty()) {
        return 0;
    }

    const auto it = std::lower_bound(state.cumulativeEntries.begin(),
                                     state.cumulativeEntries.end(),
                                     processed + 1);
    if (it == state.cumulativeEntries.end()) {
        return state.cumulativeEntries.size() - 1;
    }
    return static_cast<size_t>(std::distance(state.cumulativeEntries.begin(), it));
}

// Lightweight progress display for the EventTree -> histogram path.
// This is separate from the raw-bin monitor because the tree reader path
// has no event-build buffer or built-event chunk queue to display.
void EventTreeMonitorThread(const EventTreeProgressState& state)
{
    using namespace std::chrono_literals;

    const Long64_t total = std::max<Long64_t>(state.total, 1);
    const int width = std::to_string(total).size();

    while (!state.done.load(std::memory_order_relaxed)) {
        const Long64_t processed = std::min(state.processed.load(std::memory_order_relaxed), total);
        const std::string bar = make_queue_bar(static_cast<size_t>(processed),
                                               static_cast<size_t>(total),
                                               24);

        std::cout << "\r\033[K"
                  << "H " << CLR_GREEN << "[" << bar << "]" << CLR_RESET
                  << std::setw(width) << processed << "/" << total;

        if (!state.inputFiles.empty() && state.inputFiles.size() == state.cumulativeEntries.size()) {
            const size_t fileIndex = FindCurrentFileIndex(state, processed);
            std::cout << " | "
                      << StripFileName(state.inputFiles[fileIndex])
                      << " " << (fileIndex + 1) << "/" << state.inputFiles.size();
        }

        std::cout << std::flush;
        std::this_thread::sleep_for(400ms);
    }

    const Long64_t processed = std::min(state.processed.load(std::memory_order_relaxed), total);
    const std::string bar = make_queue_bar(static_cast<size_t>(processed),
                                           static_cast<size_t>(total),
                                           24);

    std::cout << "\r\033[K"
              << "H " << CLR_GREEN << "[" << bar << "]" << CLR_RESET
              << std::setw(width) << processed << "/" << total;

    if (!state.inputFiles.empty() && state.inputFiles.size() == state.cumulativeEntries.size()) {
        const size_t fileIndex = FindCurrentFileIndex(state, processed);
        std::cout << " | "
                  << StripFileName(state.inputFiles[fileIndex])
                  << " " << (fileIndex + 1) << "/" << state.inputFiles.size();
    }

    std::cout << std::endl;
}

// Workers keep a local counter and only publish progress periodically to
// avoid paying for an atomic increment on every processed built event.
void FlushProcessedEntries(std::atomic<Long64_t>& processed, Long64_t& localCount)
{
    if (localCount > 0) {
        processed.fetch_add(localCount, std::memory_order_relaxed);
        localCount = 0;
    }
}

// The tree progress monitor only makes sense when we know both the total
// entry count and the per-file cumulative entry boundaries from gIO.
bool ShouldShowEventTreeMonitor(TTree* tree, Long64_t totalEntries)
{
    if (!tree || totalEntries <= 0) {
        return false;
    }
    if (gIO == nullptr) {
        return false;
    }
    return !gIO->Entries.empty() && !gIO->EventInputFiles.empty();
}

}

void FillHistogramsFromEventTree(TTree* tree,
                                 ThreadedHistogramSet& histograms,
                                 unsigned int nthreads)
{
    if (!tree) {
        return;
    }

    const Long64_t nentries = tree->GetEntries();
    EventTreeProgressState progress;
    progress.total = nentries;
    if (gIO != nullptr) {
        progress.inputFiles = gIO->EventInputFiles;
        progress.cumulativeEntries.reserve(gIO->Entries.size());
        for (long entryCount : gIO->Entries) {
            progress.cumulativeEntries.push_back(static_cast<Long64_t>(entryCount));
        }
    }

    const bool showMonitor = ShouldShowEventTreeMonitor(tree, nentries);
    std::unique_ptr<std::thread> monitorThread;
    if (showMonitor) {
        monitorThread.reset(new std::thread(EventTreeMonitorThread, std::cref(progress)));
    }

    // Single-thread fallback keeps the same logic as the MT path but avoids
    // the TTreeProcessorMT setup overhead for explicitly serial runs.
    if (nthreads <= 1) {
        std::vector<UShort_t>* ts = 0;
        std::vector<UShort_t>* mod = 0;
        std::vector<UShort_t>* ch = 0;
        std::vector<UShort_t>* adc = 0;

        tree->SetBranchAddress("Ts", &ts);
        tree->SetBranchAddress("Mod", &mod);
        tree->SetBranchAddress("Ch", &ch);
        tree->SetBranchAddress("Adc", &adc);

        HistogramRefs refs = histograms.ResolveHistogramRefs();
        Long64_t localCount = 0;

        for (Long64_t entry = 0; entry < nentries; ++entry) {
            tree->GetEntry(entry);
            const BuiltEventView event{*ts, *mod, *ch, *adc};
            FillHistograms(refs, event);
            ++localCount;
            if (localCount >= 10000) {
                FlushProcessedEntries(progress.processed, localCount);
            }
        }
        FlushProcessedEntries(progress.processed, localCount);
        progress.done = true;
        if (monitorThread) {
            monitorThread->join();
        }
        return;
    }

    // Multi-threaded tree reading is handled by ROOT, while each worker
    // resolves its own thread-local histogram references.
    ROOT::EnableImplicitMT(nthreads);
    ROOT::TTreeProcessorMT processor(*tree, nthreads);

    processor.Process([&histograms, &progress](TTreeReader& reader) {
        TTreeReaderValue<std::vector<UShort_t>> ts(reader, "Ts");
        TTreeReaderValue<std::vector<UShort_t>> mod(reader, "Mod");
        TTreeReaderValue<std::vector<UShort_t>> ch(reader, "Ch");
        TTreeReaderValue<std::vector<UShort_t>> adc(reader, "Adc");
        HistogramRefs refs = histograms.ResolveHistogramRefs();
        Long64_t localCount = 0;

        while (reader.Next()) {
            const BuiltEventView event{*ts, *mod, *ch, *adc};
            FillHistograms(refs, event);
            ++localCount;
            if (localCount >= 10000) {
                FlushProcessedEntries(progress.processed, localCount);
            }
        }
        FlushProcessedEntries(progress.processed, localCount);
    });

    progress.done = true;
    if (monitorThread) {
        monitorThread->join();
    }
}

void FillHistogramsFromBuiltEventChunkQueue(ThreadSafeQueue<BuiltEventChunkBuffer*>& queue,
                                            ThreadedHistogramSet& histograms,
                                            unsigned int nthreads)
{
    ROOT::EnableThreadSafety();
    histograms.ResolveHistogramRefs();

    // Default to one worker per hardware thread when the caller leaves the
    // worker count unset, but always keep at least one consumer alive.
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
        workers.push_back(std::thread([&queue, &histograms]() {
            HistogramRefs refs = histograms.ResolveHistogramRefs();
            BuiltEventChunkBuffer* chunk = nullptr;

            while (true) {
                const bool ok = queue.pop(chunk);
                if (!ok) {
                    break;
                }

                // Once a chunk is claimed by a worker it is no longer queued
                // backlog, so remove it from the monitor counter immediately.
                // g_QueuedBuiltEvents.fetch_sub(1, std::memory_order_relaxed);
                FillHistogramsFromBuiltEventChunkSequential(*chunk, refs);
                delete chunk;
                chunk = nullptr;
            }
        }));
    }

    for (std::thread& worker : workers) {
        worker.join();
    }
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

    outfile->cd();
    histograms.WriteAll(outfile);
    outfile->Write("", TObject::kOverwrite);
    outfile->Close();
    delete outfile;
    return true;
}
