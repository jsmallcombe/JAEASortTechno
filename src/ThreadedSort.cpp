#include <ThreadedSort.h>
#include <ThreadedHistFill.h>
#include <IO.h>

#include <array>
#include <deque>
#include <utility>

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
                               EventWriter&& writeEvent,
                               DigitiserAdcHistograms* ADChists=nullptr)
{
    const Long64_t COINC_WINDOW = tdiff;
    const size_t BUFFER_TARGET = BUFFER;
    const size_t REFILL_TARGET = std::max<size_t>(1, BUFFER / gBuildRefillDivisor);

    struct RefillSlot {
        std::vector<Event> events;
        enum class State {
            Free,
            Filling,
            Sorting,
            Ready
        } state = State::Free;
    };

    struct RefillCoordinator {
        std::mutex mutex;
        std::condition_variable cv;
        std::array<RefillSlot, 2> slots;
        std::deque<int> readyOrder;
        bool producerDone = false;
    };

    Event ev;
    Long64_t globalMaxTs = -1;
    bool inputFinished = false;

    auto SetRefillState = [&](int slotIndex, int stateValue) {
        if (slotIndex < 0) {
            return;
        }
        if (slotIndex == 0) {
            g_refill_state_a = stateValue;
        } else {
            g_refill_state_b = stateValue;
        }
    };

    auto FillSortedBlock = [&](int slotIndex, std::vector<Event>& target, size_t targetSize) {
        target.clear();
        target.reserve(std::max(target.capacity(), targetSize));

        size_t added = 0;
        while (added < targetSize && !inputFinished) {
            bool anyActive = false;

            for (auto& digiPtr : digitisers) {
                auto& digi = *digiPtr;

                bool accepted = false;
                while (!accepted) {
                    const size_t sizeBefore = target.size();
                    int count = 0;

                    while (digi.getNextEvent(ev)) {
                        if(ADChists){
                            ADChists->Fill(ev.mod, ev.ch, ev.adc);
                        }
                        target.push_back(ev);
                        ++count;
                        if (count >= static_cast<int>(CHUNK_SIZE)) {
                            break;
                        }
                    }

                    if (target.size() == sizeBefore) {
                        break;
                    }

                    anyActive = true;
                    added += target.size() - sizeBefore;

                    const Long64_t digiTs = digi.getLastTs();
                    if (globalMaxTs < 0 || digiTs >= globalMaxTs) {
                        globalMaxTs = digiTs;
                        accepted = true;
                    }

                    if (added >= targetSize && accepted) {
                        break;
                    }
                }

                if (added >= targetSize) {
                    break;
                }
            }

            if (!anyActive) {
                inputFinished = true;
            }
        }

        SetRefillState(slotIndex, static_cast<int>(RefillSlot::State::Sorting));
        std::sort(target.begin(),
                  target.end(),
                  [](const Event& a, const Event& b) {
                      return a.ts < b.ts;
                  });

        return target.size();
    };

    std::vector<Event> buildBuffer;
    buildBuffer.reserve(BUFFER_TARGET);
    size_t readCount = FillSortedBlock(-1, buildBuffer, BUFFER_TARGET);
    g_ReadCount = readCount;

    if (buildBuffer.empty()) {
        g_buffer_size = 0;
        g_idx = 0;
        g_BuiltCount = 0;
        return;
    }

    RefillCoordinator refills;

    std::thread consumerThread([&]() {
        Long64_t firstTs = -1;
        Long64_t lastTs = -1;
        size_t idx = 0;
        size_t sinceMerge = 0;
        size_t FILL_EXCESS = 0;
        size_t builtCount = 0;

        auto MergeNextRefill = [&]() {
            int slotIndex = -1;
            {
                std::unique_lock<std::mutex> lock(refills.mutex);
                refills.cv.wait(lock, [&]() {
                    return !refills.readyOrder.empty() || refills.producerDone;
                });

                if (refills.readyOrder.empty()) {
                    return static_cast<size_t>(0);
                }

                slotIndex = refills.readyOrder.front();
                refills.readyOrder.pop_front();
            }

            auto& refillBuffer = refills.slots[slotIndex].events;
            const size_t added = refillBuffer.size();
            const size_t oldSize = buildBuffer.size() - idx;
            buildBuffer.insert(buildBuffer.end(),
                               std::make_move_iterator(refillBuffer.begin()),
                               std::make_move_iterator(refillBuffer.end()));

            const auto base = buildBuffer.begin() + idx;
            std::inplace_merge(base,
                               base + oldSize,
                               buildBuffer.end(),
                               [](const Event& a, const Event& b) {
                                   return a.ts < b.ts;
                               });

            refillBuffer.clear();

            {
                std::lock_guard<std::mutex> lock(refills.mutex);
                refills.slots[slotIndex].state = RefillSlot::State::Free;
            }
            SetRefillState(slotIndex, static_cast<int>(RefillSlot::State::Free));
            refills.cv.notify_all();
            return added;
        };

        // The consumer owns the active ordered buffer and the current
        // coincidence event state. It only pauses to merge a refill block
        // that the producer has already read and locally sorted.
        while (idx < buildBuffer.size()) {
            Event& current = buildBuffer[idx++];
            ++sinceMerge;

            const Long64_t currentTs = current.ts;

            if (eventBuffer.Empty()) {
                firstTs = currentTs;
                lastTs = currentTs;
                eventBuffer.StartEvent(current);
            } else if (currentTs < lastTs) {
                std::cout << "\n[TIME RESET]\n";
                firstTs = currentTs;
                lastTs = currentTs;
                eventBuffer.StartEvent(current);
            } else if (currentTs - lastTs < COINC_WINDOW) {
                eventBuffer.AppendHit(current, firstTs);
                lastTs = currentTs;
            } else {
                writeEvent(eventBuffer);
                ++builtCount;

                firstTs = currentTs;
                lastTs = currentTs;
                eventBuffer.StartEvent(current);
            }

            if (sinceMerge >= REFILL_TARGET+FILL_EXCESS) {
                const size_t merged = MergeNextRefill();
                if (merged > REFILL_TARGET) {
                    FILL_EXCESS = merged - REFILL_TARGET;
                }
                sinceMerge = 0;

                // Compact only occasionally so the consumer keeps working on
                // one contiguous active region instead of shifting on every merge.
                if (buildBuffer.size() > 2 * BUFFER) {
                    buildBuffer.erase(buildBuffer.begin(), buildBuffer.begin() + idx);
                    idx = 0;
                }
            }

            if ((idx % 1000) == 0) {
                g_buffer_size = buildBuffer.size();
                g_idx = idx;
                g_BuiltCount = builtCount;
            }
        }

        if (!eventBuffer.Empty()) {
            writeEvent(eventBuffer);
            ++builtCount;
        }

        g_buffer_size = buildBuffer.size();
        g_idx = idx;
        g_BuiltCount = builtCount;
    });

    // The main thread becomes the producer after the initial full buffer is
    // prepared. It alternates between two refill vectors so the next block can
    // be read and sorted while the consumer is merging the previous one.
    while (true) {
        int slotIndex = -1;
        {
            std::unique_lock<std::mutex> lock(refills.mutex);
            refills.cv.wait(lock, [&]() {
                for (const RefillSlot& slot : refills.slots) {
                    if (slot.state == RefillSlot::State::Free) {
                        return true;
                    }
                }
                return false;
            });

            for (size_t i = 0; i < refills.slots.size(); ++i) {
                if (refills.slots[i].state == RefillSlot::State::Free) {
                    refills.slots[i].state = RefillSlot::State::Filling;
                    slotIndex = static_cast<int>(i);
                    break;
                }
            }
        }

        if (slotIndex < 0) {
            continue;
        }

        RefillSlot& slot = refills.slots[slotIndex];
        SetRefillState(slotIndex, static_cast<int>(RefillSlot::State::Filling));
        const size_t added = FillSortedBlock(slotIndex, slot.events, REFILL_TARGET);
        readCount += added;
        g_ReadCount = readCount;

        {
            std::lock_guard<std::mutex> lock(refills.mutex);
            if (slot.events.empty()) {
                slot.state = RefillSlot::State::Free;
                refills.producerDone = true;
            } else {
                slot.state = RefillSlot::State::Ready;
                refills.readyOrder.push_back(slotIndex);
            }
        }
        SetRefillState(slotIndex,
                       slot.events.empty()
                           ? static_cast<int>(RefillSlot::State::Free)
                           : static_cast<int>(RefillSlot::State::Ready));
        refills.cv.notify_all();

        if (slot.events.empty()) {
            break;
        }
    }

    consumerThread.join();
}

} // namespace

int ThreadedBinToTree(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,TTree* outtree,Long64_t tdiff, int CHUNK, int BufferSize,DigitiserAdcHistograms* ADChists) {
    const size_t CHUNK_SIZE = CHUNK;
    const size_t BUFFER_TARGET = BufferSize;
    const size_t REFILL_TARGET = std::max<size_t>(1, BUFFER_TARGET / gBuildRefillDivisor);
    const size_t TDIFF = tdiff;
    std::atomic<bool> doneFlag{false};
    EventTreeBuffers treeEvent;
    BindBuiltEventTreeBranches(outtree, treeEvent);

    g_buffer_size = 0;
    g_idx = 0;
    g_ReadCount = 0;
    g_BuiltCount = 0;
    g_QueuedBuiltEvents = 0;
    g_refill_state_a = 0;
    g_refill_state_b = 0;

    // Tree-only mode has no built-event queue, so the monitor shows only the
    // raw event buffer state while the producer and consumer threads run.
    std::thread monitorThread(BuildMonitorThread, 0, BUFFER_TARGET, REFILL_TARGET, std::ref(doneFlag));

    BuildEventsFromDigitisers(digitisers,
                              TDIFF,
                              BUFFER_TARGET,
                              CHUNK_SIZE,
                              treeEvent,
                              [&](EventTreeBuffers&) {
                                  outtree->Fill();
                              },
                              ADChists);

    doneFlag = true;
    monitorThread.join();
    
    return 0;
}

void MakeEventTreeFromBin(TString infilename,
                          TString outfilename,
                          Long64_t tdiff,
                          int CHUNK,
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
                                      int BufferSize,
                                      Long64_t TS_TOLERANCE,
                                      TString treeOutfilename,
                                      Long64_t histChunkEvents)
{
    DigitiserBase::SetTsTolerance(TS_TOLERANCE);
    ROOT::EnableThreadSafety();

    const bool writeTree = treeOutfilename.Length() > 0;
    const size_t bufferTarget = BufferSize;
    const size_t refillTarget = std::max<size_t>(1, bufferTarget / gBuildRefillDivisor);
    const size_t chunkSize = CHUNK;
    const size_t builtEventBudget = gThreadQueueBuiltEvents;
    const size_t chunkQueueCapacity = std::max<size_t>(1, builtEventBudget / std::max<Long64_t>(1, histChunkEvents));
    std::atomic<bool> doneFlag{false};
    g_buffer_size = 0;
    g_idx = 0;
    g_ReadCount = 0;
    g_BuiltCount = 0;
    g_QueuedBuiltEvents = 0;
    g_refill_state_a = 0;
    g_refill_state_b = 0;

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
    std::thread monitorThread(BuildMonitorThread, builtEventBudget, bufferTarget, refillTarget, std::ref(doneFlag));

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
        // g_QueuedBuiltEvents.fetch_add(1, std::memory_order_relaxed);
        chunkQueue.push(chunkBuffer);
        g_QueuedBuiltEvents=chunkQueue.size();
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
        DigitiserAdcHistograms ADChists=BuildDigitiserAdcHistograms(digitisers);
        TTree *outtree = new TTree("EventTree","EventTree");
        outtree->SetDirectory(outfile);

        outtree->SetMaxTreeSize(1900LL * 1024 * 1024);
        outtree->SetAutoSave(0);

        ThreadedBinToTree(digitisers, outtree, tdiff, CHUNK, BufferSize,&ADChists);
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
                                             BufferSize,
                                             TS_TOLERANCE,
                                             writeTree ? eventTreeOutfilename : "",
                                             histChunkEvents);
}
