#include <ThreadedSort.h>
#include <ThreadedHistFill.h>
#include <IO.h>

namespace {

struct EventTreeBuffers {
    std::vector<UShort_t> Ts;
    std::vector<UShort_t> Mod;
    std::vector<UShort_t> Ch;
    std::vector<UShort_t> Adc;

    bool Empty() const
    {
        return Ts.empty();
    }

    void Clear()
    {
        Ts.clear();
        Mod.clear();
        Ch.clear();
        Adc.clear();
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

// Bind the live event-building vectors directly to a TTree. The vector
// objects stay at stable addresses for ROOT while their contents are
// cleared and refilled for each coincidence event.
void BindBuiltEventTreeBranches(TTree* tree, EventTreeBuffers& event)
{
    tree->Branch("Ts",  &event.Ts);
    tree->Branch("Mod", &event.Mod);
    tree->Branch("Ch",  &event.Ch);
    tree->Branch("Adc", &event.Adc);
}

// Create one chunk buffer that mirrors the same branch-style interface as
// the output tree, so the builder can write tree and histogram handoff data
// through the same event vectors with minimal extra logic.
BuiltEventChunkBuffer* CreateBuiltEventChunkBuffer(EventTreeBuffers& event)
{
    BuiltEventChunkBuffer* chunk = new BuiltEventChunkBuffer();
    chunk->Branch("Ts", &event.Ts);
    chunk->Branch("Mod", &event.Mod);
    chunk->Branch("Ch", &event.Ch);
    chunk->Branch("Adc", &event.Adc);
    return chunk;
}

template <typename EventWriter>
void BuildEventsFromDigitisers(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                               Long64_t tdiff,
                               size_t BUFFER,
                               size_t CHUNK_SIZE,
                               EventTreeBuffers& eventBuffer,
                               EventWriter&& writeEvent)
{
    const Long64_t COINC_WINDOW = tdiff;
    const size_t BUFFER_TARGET = BUFFER;
    const size_t REFILL_THRESHOLD = BUFFER * 0.1;

    std::vector<Event> buffer;
    buffer.reserve(BUFFER_TARGET);

    // =========================
    Event ev;

    Long64_t globalMaxTs = -1;
    bool inputFinished = false;

    // Refill the sort buffer from the raw digitisers while preserving the
    // existing globalMaxTs acceptance rule that keeps chunked bin reads
    // close to time-order before the buffered merge step.
    auto FillBuffer = [&](size_t nFill) {
        size_t added = 0;
        while (added < nFill && !inputFinished) {
            bool anyActive = false;

            for (auto& digiPtr : digitisers) {
                auto& digi = *digiPtr;

                bool accepted = false;
                while (!accepted) {
                    const size_t bufferSizeBefore = buffer.size();
                    int count = 0;

                    while (digi.getNextEvent(ev)) {
                        buffer.push_back(ev);
                        ++count;
                        if (count >= static_cast<int>(CHUNK_SIZE)) {
                            break;
                        }
                    }

                    if (buffer.size() == bufferSizeBefore) {
                        break;
                    }

                    anyActive = true;
                    added += buffer.size() - bufferSizeBefore;

                    Long64_t digiTs = digi.getLastTs();
                    if (globalMaxTs < 0 || digiTs >= globalMaxTs) {
                        globalMaxTs = digiTs;
                        accepted = true;
                    } else {
                        // Keep reading this digitiser until it catches up, matching
                        // the original producer-side chunk acceptance policy.
                    }

                    if (added >= nFill && accepted) {
                        break;
                    }
                }

                if (added >= nFill) {
                    break;
                }
            }

            if (!anyActive) {
                inputFinished = true;
            }
        }

        return added;
    };

    size_t builtCount = 0;
    size_t readCount = 0;

    readCount += FillBuffer(BUFFER_TARGET);

    std::sort(buffer.begin(), buffer.end(),
              [](const Event& a, const Event& b) {
                  return a.ts < b.ts;
              });

    Long64_t firstTs = -1;
    Long64_t lastTs = -1;
    size_t idx = 0;
    size_t popCount = 0;

    // The main builder loop walks the partially sorted buffer, groups hits
    // into coincidence events, and hands each completed event to the caller.
    while (idx < buffer.size()) {
        Event& ev = buffer[idx++];
        ++popCount;

        Long64_t tTs_local = ev.ts;

        if (eventBuffer.Empty()) {
            firstTs = tTs_local;
            lastTs = tTs_local;
            eventBuffer.StartEvent(ev);
        } else {
            if (tTs_local < lastTs) {
                std::cout << "[TIME RESET]\n";
                firstTs = tTs_local;
                lastTs = tTs_local;
                eventBuffer.StartEvent(ev);
            } else if (tTs_local - lastTs < COINC_WINDOW) {
                eventBuffer.AppendHit(ev, firstTs);
                lastTs = tTs_local;
            } else {
                writeEvent(eventBuffer);
                ++builtCount;

                firstTs = tTs_local;
                lastTs = tTs_local;
                eventBuffer.StartEvent(ev);
            }
        }

        if (popCount >= REFILL_THRESHOLD) {
            size_t old_size = buffer.size() - idx;
            size_t added = FillBuffer(BUFFER_TARGET - old_size);
            readCount += added;

            if (added > 0) {
                size_t new_size = buffer.size() - idx;
                auto base = buffer.begin() + idx;

                std::sort(base + old_size,
                          base + new_size,
                          [](const Event& a, const Event& b) {
                              return a.ts < b.ts;
                          });

                std::inplace_merge(base,
                                   base + old_size,
                                   base + new_size);
            }

            popCount = 0;

            // Compact only occasionally so we do not pay an erase/shift cost
            // on every refill.
            if (buffer.size() > 2 * BUFFER) {
                buffer.erase(buffer.begin(), buffer.begin() + idx);
                idx = 0;
            }
        }

        if (idx % 10000 == 0) {
            g_buffer_size = buffer.size();
            g_idx = idx;
            g_popCount = popCount;
            g_ReadCount = readCount;
            g_BuiltCount = builtCount;
        }
    }

    if (!eventBuffer.Empty()) {
        writeEvent(eventBuffer);
        ++builtCount;
    }
}

} // namespace

int ThreadedBinToTree(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,TTree* outtree,Long64_t tdiff, int CHUNK, int BufferSize ) {
    const size_t CHUNK_SIZE = CHUNK;
    const size_t BUFFER_TARGET = BufferSize;
    const size_t TDIFF = tdiff;
    std::atomic<bool> doneFlag{false};
    EventTreeBuffers treeEvent;
    BindBuiltEventTreeBranches(outtree, treeEvent);

    g_buffer_size = 0;
    g_idx = 0;
    g_popCount = 0;
    g_ReadCount = 0;
    g_BuiltCount = 0;
    g_QueuedBuiltEvents = 0;

    // Tree-only mode has no built-event queue, so the monitor shows only the
    // raw event buffer state while the builder runs on the main thread.
    std::thread monitorThread(BuildMonitorThread, 0, BUFFER_TARGET, std::ref(doneFlag));

    BuildEventsFromDigitisers(digitisers,
                              TDIFF,
                              BUFFER_TARGET,
                              CHUNK_SIZE,
                              treeEvent,
                              [&](EventTreeBuffers&) {
                                  outtree->Fill();
                              });

    doneFlag = true;
    monitorThread.join();
    
    return 0;
}

void MakeEventTreeFromBin(TString infilename,
                          TString outfilename,
                          Long64_t tdiff,
                          int CHUNK,
                          int QueueSize,
                          int BufferSize,
                          Long64_t TS_TOLERANCE)
{
    std::vector<std::unique_ptr<DigitiserBase>> digitisers = BuildDigitiserList(infilename);

    DigitiserBase::SetTsTolerance(TS_TOLERANCE);

    if(!outfilename.Length())outfilename=infilename + "_events.root";
    TFile *outfile = new TFile(outfilename,"RECREATE");
    TTree *outtree = new TTree("EventTree","EventTree");
    outtree->SetDirectory(outfile);

    outtree->SetMaxTreeSize(1900LL * 1024 * 1024);
    outtree->SetAutoSave(0);

    ThreadedBinToTree(digitisers, outtree, tdiff, CHUNK, BufferSize);

    TFile* currentFile = outtree->GetCurrentFile();
    if (currentFile) {
        currentFile->Write("", TObject::kOverwrite);
        currentFile->Close();
    }
}

int MakeEventTreeAndHistogramsFromBin(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                                      TString histogramOutfilename,
                                      Long64_t tdiff,
                                      unsigned int histWorkers,
                                      int CHUNK,
                                      int QueueSize,
                                      int BufferSize,
                                      Long64_t TS_TOLERANCE,
                                      TString treeOutfilename,
                                      Long64_t histChunkEvents)
{
    DigitiserBase::SetTsTolerance(TS_TOLERANCE);
    ROOT::EnableThreadSafety();

    const bool writeTree = treeOutfilename.Length() > 0;
    const size_t bufferTarget = BufferSize;
    const size_t chunkSize = CHUNK;
    const size_t builtEventBudget = static_cast<size_t>(QueueSize);
    const size_t chunkQueueCapacity = std::max<size_t>(1, builtEventBudget / std::max<Long64_t>(1, histChunkEvents));
    std::atomic<bool> doneFlag{false};
    g_buffer_size = 0;
    g_idx = 0;
    g_popCount = 0;
    g_ReadCount = 0;
    g_BuiltCount = 0;
    g_QueuedBuiltEvents = 0;

    std::unique_ptr<TFile> treeFile;
    TTree* tree = nullptr;

    if (writeTree) {
        treeFile.reset(TFile::Open(treeOutfilename, "RECREATE"));
        if (!treeFile || treeFile->IsZombie()) {
            std::cerr << "Could not create output tree file " << treeOutfilename << '\n';
            return 5;
        }

        tree = new TTree("EventTree", "EventTree");
        tree->SetDirectory(treeFile.get());
        tree->SetMaxTreeSize(1900LL * 1024 * 1024);
        tree->SetAutoSave(0);
    }

    // The histogram handoff queue carries whole built-event chunks rather
    // than individual events so the producer pays one push per chunk and the
    // worker pool gets coarse-grained work items.
    ThreadSafeQueue<BuiltEventChunkBuffer*> chunkQueue(chunkQueueCapacity);
    ThreadedHistogramSet histograms;
    std::thread monitorThread(BuildMonitorThread, builtEventBudget, bufferTarget, std::ref(doneFlag));

    std::thread histogramConsumer([&chunkQueue, &histograms, histWorkers]() {
        FillHistogramsFromBuiltEventChunkQueue(chunkQueue, histograms, histWorkers);
    });

    EventTreeBuffers treeEvent;
    if (writeTree && tree != nullptr) {
        BindBuiltEventTreeBranches(tree, treeEvent);
    }

    // Chunk rollover is now based on an exact built-event count rather than
    // an estimated byte size, which keeps the queue budget predictable.
    const Long64_t chunkTargetEvents = histChunkEvents > 0 ? histChunkEvents : gHistChunkDefaultEvents;
    BuiltEventChunkBuffer* chunkBuffer = CreateBuiltEventChunkBuffer(treeEvent);
    auto queueCompletedChunk = [&]() {
        if (!chunkBuffer || chunkBuffer->Empty()) {
            return;
        }
        g_QueuedBuiltEvents.fetch_add(chunkBuffer->Size(), std::memory_order_relaxed);
        chunkQueue.push(chunkBuffer);
        chunkBuffer = nullptr;
    };

    BuildEventsFromDigitisers(digitisers,
                              tdiff,
                              bufferTarget,
                              chunkSize,
                              treeEvent,
                              [&](EventTreeBuffers&) {
                                  if (writeTree && tree != nullptr) {
                                      tree->Fill();
                                  }

                                  // Move the just-written event vectors into
                                  // the current histogram chunk without rebinding
                                  // the tree branches away from treeEvent.
                                  chunkBuffer->FillMove();

                                  if (static_cast<Long64_t>(chunkBuffer->Size()) >= chunkTargetEvents) {
                                      queueCompletedChunk();
                                      chunkBuffer = CreateBuiltEventChunkBuffer(treeEvent);
                                  }
                              });

    queueCompletedChunk();
    if (chunkBuffer != nullptr) {
        delete chunkBuffer;
    }
    chunkQueue.set_finished();

    histogramConsumer.join();
    doneFlag = true;
    monitorThread.join();

    if (writeTree && tree != nullptr) {
        TFile* currentFile = tree->GetCurrentFile();
        if (currentFile) {
            currentFile->Write("", TObject::kOverwrite);
            currentFile->Close();
        }
    }

    if (!WriteHistogramFile(histograms, histogramOutfilename, true)) {
        return 5;
    }

    if (writeTree) {
        std::cout << "Wrote event tree to " << treeOutfilename << '\n';
    }
    std::cout << "Wrote histograms to " << histogramOutfilename << '\n';
    return 0;
}

int ThreadedSort(TChain* eventData,
                 TString histogramOutfilename,
                 unsigned int histWorkers,
                 bool overwrite)
{
    if (!eventData) {
        return 0;
    }
    if (!TestOutputPath(histogramOutfilename, overwrite, "Histogram")) {
        return 3;
    }

    ThreadedHistogramSet histograms;
    FillHistogramsFromEventTree(eventData, histograms, histWorkers);

    if (!WriteHistogramFile(histograms, histogramOutfilename, true)) {
        return 4;
    }

    std::cout << "Wrote histograms to " << histogramOutfilename << '\n';
    return 0;
}

int ThreadedSort(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                 TString eventTreeOutfilename,
                 TString histogramOutfilename,
                 Long64_t tdiff,
                 bool overwrite,
                 bool writeTree,
                 bool doHistSort,
                 unsigned int histWorkers,
                 int CHUNK,
                 int QueueSize,
                 int BufferSize,
                 Long64_t TS_TOLERANCE,
                 Long64_t histChunkEvents)
{
    if (!writeTree && !doHistSort) {
        return 0;
    }

    if (writeTree && !TestOutputPath(eventTreeOutfilename, overwrite, "TTree")) {
        return 3;
    }
    if (doHistSort && !TestOutputPath(histogramOutfilename, overwrite, "Histogram")) {
        return 4;
    }

    if (writeTree && !doHistSort) {
        DigitiserBase::SetTsTolerance(TS_TOLERANCE);

        TFile *outfile = new TFile(eventTreeOutfilename,"RECREATE");
        TTree *outtree = new TTree("EventTree","EventTree");
        outtree->SetDirectory(outfile);

        outtree->SetMaxTreeSize(1900LL * 1024 * 1024);
        outtree->SetAutoSave(0);

        ThreadedBinToTree(digitisers, outtree, tdiff, CHUNK, BufferSize);

        TFile* currentFile = outtree->GetCurrentFile();
        if (currentFile) {
            currentFile->Write("", TObject::kOverwrite);
            currentFile->Close();
        }
        return 0;
    }

    return MakeEventTreeAndHistogramsFromBin(digitisers,
                                             histogramOutfilename,
                                             tdiff,
                                             histWorkers,
                                             CHUNK,
                                             QueueSize,
                                             BufferSize,
                                             TS_TOLERANCE,
                                             writeTree ? eventTreeOutfilename : "",
                                             histChunkEvents);
}
