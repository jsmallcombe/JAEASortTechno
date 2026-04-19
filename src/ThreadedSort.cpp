#include <ThreadedSort.h>
#include <BuildEventsFromDigitisers.h>
#include <ThreadedHistFill.h>
#include <IO.h>
#include <utility>



namespace {

//// Helper structures and functions for the threaded event-building and histogram-filling logic. These are defined in the .cpp file since they are only relevant to the implementation of the threaded sort and not needed by other code.

// Bind the live event-building vectors directly to a TTree. The vector
// objects stay at stable addresses for ROOT while their contents are
// cleared and refilled for each coincidence event.
void BindBuiltEventTreeBranches(TTree* tree, BuiltEvent& event)
{
    tree->Branch("Ts",  &event.Ts);
    tree->Branch("Mod", &event.Mod);
    tree->Branch("Ch",  &event.Ch);
    tree->Branch("Adc", &event.Adc);
}

struct EventTreeOutput {
    std::unique_ptr<TFile> file;
    TTree* tree = nullptr;
};

EventTreeOutput CreateEventTreeOutput(const TString& outfilename)
{
    EventTreeOutput output;
    output.file.reset(TFile::Open(outfilename, "RECREATE"));
    if (!output.file || output.file->IsZombie()) {
        return output;
    }

    output.tree = new TTree("EventTree", "EventTree");
    output.tree->SetDirectory(output.file.get());
    output.tree->SetMaxTreeSize(1900LL * 1024 * 1024);
    output.tree->SetAutoSave(0);
    return output;
}

bool WriteAndCloseCurrentTreeFile(TTree* tree)
{
    if (tree == nullptr) {
        return false;
    }

    TFile* currentFile = tree->GetCurrentFile();
    if (currentFile == nullptr) {
        return false;
    }

    currentFile->Write("", TObject::kOverwrite);
    currentFile->Close();
    return true;
}

int ThreadedBinToTree(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                      TTree* outtree,
                      Long64_t tdiff,
                      int CHUNK,
                      int BufferSize,
                      DigitiserAdcHistograms* ADChists = nullptr)
{
    const size_t CHUNK_SIZE = CHUNK;
    const size_t BUFFER_TARGET = BufferSize;
    const size_t REFILL_TARGET = std::max<size_t>(1, BUFFER_TARGET / gBuildRefillDivisor);
    const size_t TDIFF = tdiff;
    std::atomic<bool> doneFlag{false};
    BuiltEvent treeEvent;
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
                              [&](BuiltEvent&) {
                                  outtree->Fill();
                              },
                              ADChists);

    doneFlag = true;
    monitorThread.join();

    return 0;
}

int MakeEventTreeAndHistogramsFromBin(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                                      TString histogramOutfilename,
                                      Long64_t tdiff,
                                      unsigned int histWorkers,
                                      int CHUNK,
                                      int BufferSize,
                                      TString treeOutfilename,
                                      Long64_t histChunkEvents)
{
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

    EventTreeOutput treeOutput;
    if (writeTree) {
        treeOutput = CreateEventTreeOutput(treeOutfilename);
        if (!treeOutput.file || treeOutput.tree == nullptr) {
            std::cerr << "Could not create output tree file " << treeOutfilename << '\n';
            return 5;
        }
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

    BuiltEvent treeEvent;
    if (writeTree && treeOutput.tree != nullptr) {
        BindBuiltEventTreeBranches(treeOutput.tree, treeEvent);
    }

    // Chunk rollover is now based on an exact built-event count rather than
    // an estimated byte size, which keeps the queue budget predictable.
    const Long64_t chunkTargetEvents = histChunkEvents > 0 ? histChunkEvents : gHistChunkDefaultEvents;
    BuiltEventChunkBuffer* chunkBuffer = CreateBuiltEventChunkBuffer(treeEvent);
    auto queueCompletedChunk = [&]() {
        if (!chunkBuffer || chunkBuffer->Empty()) {
            return;
        }
        chunkQueue.push(chunkBuffer);
        g_QueuedBuiltEvents = chunkQueue.size();
        chunkBuffer = nullptr;
    };

    BuildEventsFromDigitisers(digitisers,
                              tdiff,
                              bufferTarget,
                              chunkSize,
                              treeEvent,
                              [&](BuiltEvent&) {
                                  if (writeTree && treeOutput.tree != nullptr) {
                                      treeOutput.tree->Fill();
                                  }

                                  // Move the just-written event vectors into
                                  // the current histogram chunk without rebinding
                                  // the tree branches away from treeEvent.
                                  chunkBuffer->FillMove();

                                  if (static_cast<Long64_t>(chunkBuffer->Size()) >= chunkTargetEvents) {
                                      queueCompletedChunk();
                                      chunkBuffer = CreateBuiltEventChunkBuffer(treeEvent);
                                  }
                              },
                              nullptr);

    queueCompletedChunk();
    chunkQueue.set_finished();

    histogramConsumer.join();
    doneFlag = true;
    monitorThread.join();
    
    if (chunkBuffer != nullptr) {
        delete chunkBuffer;
    }

    if (writeTree && treeOutput.tree != nullptr) {
        WriteAndCloseCurrentTreeFile(treeOutput.tree);
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


} // namespace

void MakeEventTreeFromBin(TString infilename,
                          TString outfilename,
                          Long64_t tdiff,
                          int CHUNK,
                          int BufferSize,
                          Long64_t TS_TOLERANCE)
{
    std::vector<std::unique_ptr<DigitiserBase>> digitisers = BuildDigitiserList(infilename);

    DigitiserBase::SetTsTolerance(TS_TOLERANCE);

    if (!outfilename.Length()) {
        outfilename = infilename + "_events.root";
    }

    EventTreeOutput treeOutput = CreateEventTreeOutput(outfilename);
    if (!treeOutput.file || treeOutput.tree == nullptr) {
        std::cerr << "Could not create output tree file " << outfilename << '\n';
        return;
    }

    ThreadedBinToTree(digitisers, treeOutput.tree, tdiff, CHUNK, BufferSize);
    WriteAndCloseCurrentTreeFile(treeOutput.tree);
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

    DigitiserBase::SetTsTolerance(TS_TOLERANCE);
    
    if (writeTree && !doHistSort) {
        DigitiserBase::SetTsTolerance(TS_TOLERANCE);

        DigitiserAdcHistograms ADChists = BuildDigitiserAdcHistograms(digitisers);
        EventTreeOutput treeOutput = CreateEventTreeOutput(eventTreeOutfilename);
        if (!treeOutput.file || treeOutput.tree == nullptr) {
            std::cerr << "Could not create output tree file " << eventTreeOutfilename << '\n';
            return 5;
        }else{
            ADChists.SetDirectory(treeOutput.file.get());
        }

        ThreadedBinToTree(digitisers, treeOutput.tree, tdiff, CHUNK, BufferSize, &ADChists);
        WriteAndCloseCurrentTreeFile(treeOutput.tree);
        return 0;
    }

    return MakeEventTreeAndHistogramsFromBin(digitisers,
                                             histogramOutfilename,
                                             tdiff,
                                             histWorkers,
                                             CHUNK,
                                             BufferSize,
                                             writeTree ? eventTreeOutfilename : "",
                                             histChunkEvents);
}
