#include <ThreadedSort.h>
#include <FillHistograms.h>
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

void BindBuiltEventTreeBranches(TTree* tree, EventTreeBuffers& event)
{
    tree->Branch("Ts",  &event.Ts);
    tree->Branch("Mod", &event.Mod);
    tree->Branch("Ch",  &event.Ch);
    tree->Branch("Adc", &event.Adc);
}

void BindBuiltEventTreeBranches(TTree* tree, BuiltEvent& event)
{
    tree->Branch("Ts",  &event.Ts);
    tree->Branch("Mod", &event.Mod);
    tree->Branch("Ch",  &event.Ch);
    tree->Branch("Adc", &event.Adc);
}

TMemFile* CreateEventTreeChunk(unsigned int chunkIndex, EventTreeBuffers& event)
{
    TString memName;
    memName.Form("EventTreeChunk_%u.root", chunkIndex);

    TMemFile* memFile = new TMemFile(memName, "RECREATE");
    TTree* tree = new TTree("EventTree", "EventTree");
    tree->SetDirectory(memFile);
    tree->SetAutoSave(0);
    BindBuiltEventTreeBranches(tree, event);
    return memFile;
}

void FinalizeEventTreeChunk(TMemFile* memFile)
{
    if (!memFile) {
        return;
    }
    TTree* tree = dynamic_cast<TTree*>(memFile->Get("EventTree"));
    if (tree) {
        tree->FlushBaskets();
    }
    memFile->Write("", TObject::kOverwrite);
}

struct BuildStageTimers {
    std::chrono::steady_clock::duration refill{};
    std::chrono::steady_clock::duration sort{};
    std::chrono::steady_clock::duration sink{};
    std::chrono::steady_clock::duration diskTreeFill{};
    std::chrono::steady_clock::duration chunkTreeFill{};
    std::chrono::steady_clock::duration chunkFinalize{};
    size_t chunkCount = 0;

    void Print(const char* label,
               size_t eventsRead,
               size_t eventsBuilt,
               size_t maxBuffered) const
    {
        using namespace std::chrono;
        const double refillSec = duration<double>(refill).count();
        const double sortSec = duration<double>(sort).count();
        const double sinkSec = duration<double>(sink).count();
        const double diskTreeFillSec = duration<double>(diskTreeFill).count();
        const double chunkTreeFillSec = duration<double>(chunkTreeFill).count();
        const double chunkFinalizeSec = duration<double>(chunkFinalize).count();
        const double totalSec = refillSec + sortSec + sinkSec;
        const double bufferMiB = (maxBuffered * sizeof(Event)) / (1024.0 * 1024.0);

        std::cout << "\n[" << label << " stage timings]\n"
                  << "  refill/arbitrate : " << refillSec << " s\n"
                  << "  sort/merge       : " << sortSec << " s\n"
                  << "  build/sink       : " << sinkSec << " s\n"
                  << "  subtotal         : " << totalSec << " s\n"
                  << "  disk tree fill   : " << diskTreeFillSec << " s\n"
                  << "  chunk tree fill  : " << chunkTreeFillSec << " s\n"
                  << "  chunk finalize   : " << chunkFinalizeSec << " s\n"
                  << "  events read      : " << eventsRead << "\n"
                  << "  events built     : " << eventsBuilt << "\n"
                  << "  chunk count      : " << chunkCount << "\n"
                  << "  max buffered     : " << maxBuffered
                  << " (" << bufferMiB << " MiB of Event storage)\n";
    }
};

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
    BuildStageTimers timers;
    size_t maxBuffered = 0;

    // =========================
    // FillBuffer replacement
    // =========================
    auto FillBuffer = [&](size_t nFill) {
        size_t added = 0;
        const auto refillStart = std::chrono::steady_clock::now();

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

        timers.refill += std::chrono::steady_clock::now() - refillStart;
        if (buffer.size() > maxBuffered) {
            maxBuffered = buffer.size();
        }
        return added;
    };

    size_t builtCount = 0;
    size_t readCount = 0;

    readCount += FillBuffer(BUFFER_TARGET);

    auto sortStart = std::chrono::steady_clock::now();
    std::sort(buffer.begin(), buffer.end(),
              [](const Event& a, const Event& b) {
                  return a.ts < b.ts;
              });
    timers.sort += std::chrono::steady_clock::now() - sortStart;

    Long64_t firstTs = -1;
    Long64_t lastTs = -1;
    size_t idx = 0;
    size_t popCount = 0;

    // =========================
    // Main loop
    // =========================
    while (idx < buffer.size()) {
        Event& ev = buffer[idx++];
        ++popCount;

        Long64_t tTs_local = ev.ts;

        // --- event building ---
        if (eventBuffer.Empty()) {
            // Only actually called for very first event
            firstTs = tTs_local;
            lastTs = tTs_local;
            eventBuffer.StartEvent(ev);
        } else {
            // Sort failure state, should never happen
            if (tTs_local < lastTs) {
                std::cout << "[TIME RESET]\n";
                firstTs = tTs_local;
                lastTs = tTs_local;
                eventBuffer.StartEvent(ev);
            } else if (tTs_local - lastTs < COINC_WINDOW) {
                // Is part of current event, add
                eventBuffer.AppendHit(ev, firstTs);
                lastTs = tTs_local;
            } else {
                // EVENT COMPLETE
                auto sinkStart = std::chrono::steady_clock::now();
                writeEvent(eventBuffer);
                timers.sink += std::chrono::steady_clock::now() - sinkStart;
                ++builtCount;

                // Start next event
                firstTs = tTs_local;
                lastTs = tTs_local;
                eventBuffer.StartEvent(ev);
            }
        }

        // =========================
        // Refill condition
        // =========================
        if (popCount >= REFILL_THRESHOLD) {
            ///// remove consumed elements on every read
            // buffer.erase(buffer.begin(), buffer.begin() + idx);
            // idx = 0;

            size_t old_size = buffer.size() - idx;
            size_t added = FillBuffer(BUFFER_TARGET - old_size);
            readCount += added;

            if (added > 0) {
                size_t new_size = buffer.size() - idx;
                auto base = buffer.begin() + idx;

                // Sort ONLY new elements
                sortStart = std::chrono::steady_clock::now();
                std::sort(base + old_size,
                          base + new_size,
                          [](const Event& a, const Event& b) {
                              return a.ts < b.ts;
                          });

                // Merge only as far as current index
                std::inplace_merge(base,
                                   base + old_size,
                                   base + new_size);
                timers.sort += std::chrono::steady_clock::now() - sortStart;
            }

            popCount = 0;

            // =========================
            // Occasional compaction. Not notably faster than shrinking every fill
            // =========================
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
        auto sinkStart = std::chrono::steady_clock::now();
        writeEvent(eventBuffer);
        timers.sink += std::chrono::steady_clock::now() - sinkStart;
        ++builtCount;
    }
    timers.Print("BuildEventsFromDigitisers", readCount, builtCount, maxBuffered);
}

template <typename Sink>
void BuildEventsFromThreadedQueue(ThreadSafeQueue<std::vector<Event>>& queue,
                                  Long64_t tdiff,
                                  size_t BUFFER,
                                  Sink&& sink)
{
    const Long64_t COINC_WINDOW = tdiff;
    const size_t BUFFER_TARGET = BUFFER;
    const size_t REFILL_THRESHOLD = BUFFER * 0.1;

    std::vector<Event> buffer;
    buffer.reserve(BUFFER_TARGET);

    BuiltEvent builtEvent;

    auto reset_built_event = [&](const Event& ev) {
        builtEvent.Clear();
        builtEvent.Ts.push_back(0);
        builtEvent.Mod.push_back(ev.mod);
        builtEvent.Ch.push_back(ev.ch);
        builtEvent.Adc.push_back(ev.adc);
    };

    auto flush_built_event = [&]() {
        if (builtEvent.Empty()) {
            return;
        }
        sink(std::move(builtEvent));
        builtEvent = BuiltEvent{};
    };

    auto FillBuffer = [&](size_t nFill) {
        size_t added = 0;

        while (added < nFill) {
            std::vector<Event> chunk;

            if (!queue.pop(chunk)) {
                break;
            }

            for (auto& ev : chunk) {
                buffer.push_back(std::move(ev));
                ++added;
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

    while (idx < buffer.size()) {
        Event& ev = buffer[idx++];
        ++popCount;

        Long64_t tTs_local = ev.ts;

        if (builtEvent.Empty()) {
            firstTs = tTs_local;
            lastTs = tTs_local;
            reset_built_event(ev);
        } else {
            if (tTs_local < lastTs) {
                std::cout << "[TIME RESET]\n";
                firstTs = tTs_local;
                lastTs = tTs_local;
                reset_built_event(ev);
            } else if (tTs_local - lastTs < COINC_WINDOW) {
                builtEvent.Ts.push_back(SafeTsDiff(tTs_local, firstTs));
                builtEvent.Mod.push_back(ev.mod);
                builtEvent.Ch.push_back(ev.ch);
                builtEvent.Adc.push_back(ev.adc);
                lastTs = tTs_local;
            } else {
                flush_built_event();
                ++builtCount;

                firstTs = tTs_local;
                lastTs = tTs_local;
                reset_built_event(ev);
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

    flush_built_event();
}

} // namespace

// // producer
void BinToThreadedQueue(ThreadSafeQueue<std::vector<Event>>& queue,
              std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
              size_t CHUNK_SIZE)
{
    Event ev;
    std::vector<Event> chunk;
    chunk.reserve(CHUNK_SIZE);
    
    Long64_t globalMaxTs = -1;
    bool anyActive = true;
    
    while (anyActive) {
        anyActive = false;
        
        for (auto& digiPtr : digitisers) {
            
            auto& digi = *digiPtr;
            chunk.clear();
            
            bool timeaccept=false;
            while(!timeaccept){
                timeaccept=true;
                
                int count = 0;
                bool fullread=false;
                while (digi.getNextEvent(ev)) {
                    chunk.push_back(ev);
                    count++;
                    if(count >= CHUNK_SIZE){
                        fullread=true;
                        break;
                    }
                }
                
                if (!chunk.empty()) {
                    queue.push(std::move(chunk));
                    chunk = std::vector<Event>();  // reset moved vector
                    chunk.reserve(CHUNK_SIZE);
                    if(fullread){
                        anyActive = true;
                        timeaccept=false;//read more unless time is good
                    }
                    
                    Long64_t digiTs = digi.getLastTs();
                    if (globalMaxTs < 0 || digiTs >= globalMaxTs) {
                        globalMaxTs = digiTs;
                        timeaccept = true;
                    }                
                }
            
            }
            // else → read another chunk
            
        }
    }
    
    queue.set_finished();  // 🔑 critical
}

//consumer
void ThreadedQueueToEventTree(ThreadSafeQueue<std::vector<Event>>& queue, TTree* outtree, Long64_t tdiff, size_t BUFFER)
{
    BuiltEvent treeEvent;

    BindBuiltEventTreeBranches(outtree, treeEvent);

    BuildEventsFromThreadedQueue(queue,
                                 tdiff,
                                 BUFFER,
                                 [&](BuiltEvent&& event) {
                                     treeEvent = event;
                                     outtree->Fill();
                                 });
}

void ThreadedQueueToBuiltEventQueue(ThreadSafeQueue<std::vector<Event>>& input_queue,
                                    ThreadSafeQueue<BuiltEvent>& output_queue,
                                    Long64_t tdiff,
                                    size_t BUFFER)
{
    BuildEventsFromThreadedQueue(input_queue,
                                 tdiff,
                                 BUFFER,
                                 [&](BuiltEvent&& event) {
                                     output_queue.push(std::move(event));
                                 });
    output_queue.set_finished();
}

void ThreadedQueueToEventTreeAndBuiltEventQueue(ThreadSafeQueue<std::vector<Event>>& input_queue,
                                                TTree* outtree,
                                                ThreadSafeQueue<BuiltEvent>& output_queue,
                                                Long64_t tdiff,
                                                size_t BUFFER)
{
    BuiltEvent treeEvent;

    BindBuiltEventTreeBranches(outtree, treeEvent);

    BuildEventsFromThreadedQueue(input_queue,
                                 tdiff,
                                 BUFFER,
                                 [&](BuiltEvent&& event) {
                                     treeEvent = event;
                                     outtree->Fill();
                                     output_queue.push(std::move(event));
                                 });
    output_queue.set_finished();
}


int ThreadedBinToTree(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,TTree* outtree,Long64_t tdiff, int CHUNK, int QueueSize, int BufferSize ) {
    (void)QueueSize;

    const size_t CHUNK_SIZE = CHUNK;
    const size_t BUFFER_TARGET = BufferSize;
    const size_t TDIFF = tdiff;
    EventTreeBuffers treeEvent;
    BindBuiltEventTreeBranches(outtree, treeEvent);

    BuildEventsFromDigitisers(digitisers,
                              TDIFF,
                              BUFFER_TARGET,
                              CHUNK_SIZE,
                              treeEvent,
                              [&](EventTreeBuffers&) {
                                  outtree->Fill();
                              });
    
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

    ThreadedBinToTree(digitisers, outtree, tdiff, CHUNK, QueueSize, BufferSize);

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
                                      Long64_t histTreeChunkBytes,
                                      EventTreeQueueHistMode histTreeMode)
{
    DigitiserBase::SetTsTolerance(TS_TOLERANCE);
    ROOT::EnableThreadSafety();

    const bool writeTree = treeOutfilename.Length() > 0;
    const size_t bufferTarget = BufferSize;
    const size_t chunkSize = CHUNK;
    BuildStageTimers sinkBreakdown;

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

    ThreadSafeQueue<TMemFile*> memTreeQueue(QueueSize);
    ThreadedHistogramSet histograms;

    std::thread histogramConsumer;
    if (histTreeMode == EventTreeQueueHistMode::PerChunkFillHistogramsFromEventTree) {
        histogramConsumer = std::thread([&memTreeQueue, &histograms, histWorkers]() {
            FillHistogramsFromEventTreeQueueUsingExistingFunction(memTreeQueue, histograms, histWorkers);
        });
    } else {
        histogramConsumer = std::thread([&memTreeQueue, &histograms, histWorkers]() {
            FillHistogramsFromEventTreeQueue(memTreeQueue, histograms, histWorkers);
        });
    }

    EventTreeBuffers treeEvent;
    if (writeTree && tree != nullptr) {
        BindBuiltEventTreeBranches(tree, treeEvent);
    }

    const Long64_t chunkTargetBytes = histTreeChunkBytes > 0 ? histTreeChunkBytes : (500LL * 1024 * 1024);
    unsigned int chunkIndex = 0;
    TMemFile* chunkFile = CreateEventTreeChunk(chunkIndex++, treeEvent);
    TTree* chunkTree = dynamic_cast<TTree*>(chunkFile->Get("EventTree"));

    size_t chunkEntries = 0;
    auto queueCompletedChunk = [&]() {
        if (!chunkFile || chunkEntries == 0) {
            return;
        }
        const auto finalizeStart = std::chrono::steady_clock::now();
        FinalizeEventTreeChunk(chunkFile);
        sinkBreakdown.chunkFinalize += std::chrono::steady_clock::now() - finalizeStart;
        memTreeQueue.push(chunkFile);
        ++sinkBreakdown.chunkCount;
        chunkFile = nullptr;
        chunkTree = nullptr;
        chunkEntries = 0;
    };

    BuildEventsFromDigitisers(digitisers,
                              tdiff,
                              bufferTarget,
                              chunkSize,
                              treeEvent,
                              [&](EventTreeBuffers&) {
                                  if (writeTree && tree != nullptr) {
                                      const auto diskFillStart = std::chrono::steady_clock::now();
                                      tree->Fill();
                                      sinkBreakdown.diskTreeFill += std::chrono::steady_clock::now() - diskFillStart;
                                  }

                                  const auto chunkFillStart = std::chrono::steady_clock::now();
                                  chunkTree->Fill();
                                  sinkBreakdown.chunkTreeFill += std::chrono::steady_clock::now() - chunkFillStart;
                                  ++chunkEntries;

                                  if (chunkTree->GetTotBytes() >= chunkTargetBytes) {
                                      queueCompletedChunk();
                                      chunkFile = CreateEventTreeChunk(chunkIndex++, treeEvent);
                                      chunkTree = dynamic_cast<TTree*>(chunkFile->Get("EventTree"));
                                  }
                              });

    queueCompletedChunk();
    if (chunkFile != nullptr) {
        delete chunkFile;
    }
    memTreeQueue.set_finished();

    // std::thread monitorThread(QueueMonitorThread,
    //                           std::ref(rawQueue),
    //                           maxQueue,
    //                           bufferTarget,
    //                           std::ref(doneFlag));

    histogramConsumer.join();

    std::cout << "\n[Combined sink breakdown]\n"
              << "  disk tree fill   : " << std::chrono::duration<double>(sinkBreakdown.diskTreeFill).count() << " s\n"
              << "  chunk tree fill  : " << std::chrono::duration<double>(sinkBreakdown.chunkTreeFill).count() << " s\n"
              << "  chunk finalize   : " << std::chrono::duration<double>(sinkBreakdown.chunkFinalize).count() << " s\n"
              << "  chunk count      : " << sinkBreakdown.chunkCount << "\n";

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
                 Long64_t histTreeChunkBytes,
                 EventTreeQueueHistMode histTreeMode)
{
    const bool effectiveWriteTree = writeTree || doHistSort;

    if (!effectiveWriteTree && !doHistSort) {
        return 0;
    }

    if (effectiveWriteTree && !TestOutputPath(eventTreeOutfilename, overwrite, "TTree")) {
        return 3;
    }
    if (doHistSort && !TestOutputPath(histogramOutfilename, overwrite, "Histogram")) {
        return 4;
    }

    if (effectiveWriteTree && !doHistSort) {
        DigitiserBase::SetTsTolerance(TS_TOLERANCE);

        TFile *outfile = new TFile(eventTreeOutfilename,"RECREATE");
        TTree *outtree = new TTree("EventTree","EventTree");
        outtree->SetDirectory(outfile);

        outtree->SetMaxTreeSize(1900LL * 1024 * 1024);
        outtree->SetAutoSave(0);

        ThreadedBinToTree(digitisers, outtree, tdiff, CHUNK, QueueSize, BufferSize);

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
                                             effectiveWriteTree ? eventTreeOutfilename : "",
                                             histTreeChunkBytes,
                                             histTreeMode);
}
