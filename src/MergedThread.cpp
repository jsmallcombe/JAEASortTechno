#include <MergedThread.h>

namespace {

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

    // =========================
    // FillBuffer replacement
    // =========================
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

    // =========================
    // Main loop
    // =========================
    while (idx < buffer.size()) {
        Event& ev = buffer[idx++];
        ++popCount;

        Long64_t tTs_local = ev.ts;

        // --- event building ---
        if (builtEvent.Empty()) {
            // Only actually called for very first event
            firstTs = tTs_local;
            lastTs = tTs_local;
            reset_built_event(ev);
        } else {
            // Sort failure state, should never happen
            if (tTs_local < lastTs) {
                std::cout << "[TIME RESET]\n";
                firstTs = tTs_local;
                lastTs = tTs_local;
                reset_built_event(ev);
            } else if (tTs_local - lastTs < COINC_WINDOW) {
                // Is part of current event, add
                builtEvent.Ts.push_back(SafeTsDiff(tTs_local, firstTs));
                builtEvent.Mod.push_back(ev.mod);
                builtEvent.Ch.push_back(ev.ch);
                builtEvent.Adc.push_back(ev.adc);
                lastTs = tTs_local;
            } else {
                // EVENT COMPLETE
                flush_built_event();
                ++builtCount;

                // Start next event
                firstTs = tTs_local;
                lastTs = tTs_local;
                reset_built_event(ev);
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
                std::sort(base + old_size,
                          base + new_size,
                          [](const Event& a, const Event& b) {
                              return a.ts < b.ts;
                          });

                // Merge only as far as current index
                std::inplace_merge(base,
                                   base + old_size,
                                   base + new_size);
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

    outtree->Branch("Ts",  &treeEvent.Ts);
    outtree->Branch("Mod", &treeEvent.Mod);
    outtree->Branch("Ch",  &treeEvent.Ch);
    outtree->Branch("Adc", &treeEvent.Adc);

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

    outtree->Branch("Ts",  &treeEvent.Ts);
    outtree->Branch("Mod", &treeEvent.Mod);
    outtree->Branch("Ch",  &treeEvent.Ch);
    outtree->Branch("Adc", &treeEvent.Adc);

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



int ThreadSort(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,TTree* outtree,Long64_t tdiff, int CHUNK, int QueueSize, int BufferSize ) {
    
    const size_t MAX_QUEUE = QueueSize; 
    const int CHUNK_SIZE = CHUNK;
    const size_t BUFFER_TARGET = BufferSize;
    const size_t TDIFF = tdiff;
    
    std::atomic<bool> done_flag{false};
    
    
    ThreadSafeQueue<std::vector<Event>> queue(MAX_QUEUE);
    
    std::thread t_monitor(QueueMonitorThread,
                          std::ref(queue),
                          MAX_QUEUE,
                          BUFFER_TARGET,
                          std::ref(done_flag));
    
    // Start threads
    std::thread t1(BinToThreadedQueue,
                   std::ref(queue),
                   std::ref(digitisers),
                   CHUNK_SIZE);
    
    std::thread t2(ThreadedQueueToEventTree,
                   std::ref(queue),
                   outtree,
                   TDIFF,
                   BUFFER_TARGET);
    
    // Wait for completion
    t1.join();
    t2.join();
    done_flag = true;
    t_monitor.join();
    
    return 0;
}



void MakeEventTreeFromBin(TString infilename,TString outfilename,Long64_t tdiff,int CHUNK,int QueueSize,int BufferSize,Long64_t TS_TOLERANCE){
    
    std::vector<std::unique_ptr<DigitiserBase>> digitisers=BuildDigitiserList(infilename);
    
    DigitiserBase::SetTsTolerance(TS_TOLERANCE);
    
    
    // Start timer
    TStopwatch timer;
    timer.Start();
    
    if(!outfilename.Length())outfilename=infilename + "_events.root";
    TFile *outfile = new TFile(outfilename,"RECREATE");
    TTree *outtree = new TTree("EventTree","EventTree");
    
    // ROOT auto file splitting (~1.9 GB safe)
    outtree->SetMaxTreeSize(1900LL * 1024 * 1024);
    // Removes autosave in the case of crash, but removes duplicate TTree keys
    outtree->SetAutoSave(0);
    
    ThreadSort(digitisers,outtree,tdiff,CHUNK,QueueSize,BufferSize);
        
        
    TFile* currentFile = outtree->GetCurrentFile();
    if (currentFile) {
        currentFile->Write("", TObject::kOverwrite);
        currentFile->Close();
    }
    
    std::cout << "\nDone\n";
    
    timer.Stop();
    Double_t rtime = timer.RealTime();
    Double_t ctime = timer.CpuTime();
    std::cout << Form("\n RealTime = %d seconds, CpuTime = %d seconds\n\n",(Int_t)rtime,(Int_t)ctime );
}

