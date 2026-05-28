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
    tree->Branch("Ts_l",  &event.Ts_l);
    tree->Branch("Ts",  &event.Ts);
    tree->Branch("Mod", &event.Mod);
    tree->Branch("Ch",  &event.Ch);
    tree->Branch("Adc", &event.Adc);
}

struct EventTreeOutput {
    TString filename;
    TTree* tree = nullptr;
};

EventTreeOutput CreateEventTreeOutput(const TString& outfilename)
{
    EventTreeOutput output;
    TFile* file = TFile::Open(outfilename, "RECREATE");
    if (!file || file->IsZombie()) {
        return output;
    }

    output.filename = outfilename;
    output.tree = new TTree("EventTree", "EventTree");
    output.tree->SetDirectory(file);
    output.tree->SetMaxTreeSize(1900LL * 1024 * 1024);
    output.tree->SetAutoSave(0);
    return output;
}

bool WriteAdcHistogramsToFile(const TString& outfilename, DigitiserAdcHistograms& histograms)
{
    TFile* file = TFile::Open(outfilename, "UPDATE");
    if (file == nullptr || file->IsZombie()) {
        std::cerr << "Could not reopen output tree file for ADC histograms " << outfilename << '\n';
        return false;
    }

    std::cout << "\n[SORT DEBUG] ADC histogram write starting"
              << " | file=" << outfilename
              << std::endl;
    histograms.Write();
    file->Write("", TObject::kOverwrite);
    file->Close();
    std::cout << "\n[SORT DEBUG] ADC histogram write finished"
              << " | file=" << outfilename
              << std::endl;
    return true;
}

bool WriteAndCloseTreeOutput(EventTreeOutput& output)
{
    if (output.tree == nullptr) {
        return false;
    }

    TFile* currentFile = output.tree->GetCurrentFile();
    if (currentFile == nullptr) {
        return false;
    }

    std::cout << "\n[SORT DEBUG] tree write starting"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " | entries=" << output.tree->GetEntriesFast()
              << " file=" << currentFile->GetName()
              << std::endl;
    currentFile->Write("", TObject::kOverwrite);
    std::cout << "\n[SORT DEBUG] tree write finished"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " | entries=" << output.tree->GetEntriesFast()
              << " file=" << currentFile->GetName()
              << std::endl;
    currentFile->Close();
    std::cout << "\n[SORT DEBUG] tree file close finished"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " | file=" << currentFile->GetName()
              << std::endl;

    output.tree = nullptr;
    return true;
}

int ThreadedBinToTree(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                      TTree* outtree,
                      Long64_t tdiff,
                      int CHUNK,
                      int BufferSize,
                      DigitiserAdcHistograms* ADChists = nullptr,
                      Long64_t maxSortTs = -1)
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
                              ADChists,
                              maxSortTs);
    std::cout << "\n[SORT DEBUG] BuildEventsFromDigitisers returned"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " buffer=" << g_idx.load() << "/" << g_buffer_size.load()
              << std::endl;

    doneFlag = true;
    monitorThread.join();
    std::cout << "\n[SORT DEBUG] monitor thread joined"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " buffer=" << g_idx.load() << "/" << g_buffer_size.load()
              << std::endl;

    return 0;
}

int MakeEventTreeAndHistogramsFromBin(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                                      TString histogramOutfilename,
                                      Long64_t tdiff,
                                      unsigned int histWorkers,
                                      int CHUNK,
                                      int BufferSize,
                                      TString treeOutfilename,
                                      Long64_t histChunkEvents,
                                      Long64_t maxSortTs)
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
        if (treeOutput.tree == nullptr) {
            std::cerr << "Could not create output tree file " << treeOutfilename << '\n';
            return 5;
        }
    }

    // The histogram handoff queue carries whole built-event chunks rather
    // than individual events so the producer pays one push per chunk and the
    // worker pool gets coarse-grained work items.
    ThreadSafeQueue<BuiltEventChunkBuffer*> chunkQueue(chunkQueueCapacity);
    ThreadedHistogramCollection histograms;
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
                              nullptr,
                              maxSortTs);
    std::cout << "\n[SORT DEBUG] BuildEventsFromDigitisers returned"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " buffer=" << g_idx.load() << "/" << g_buffer_size.load()
              << std::endl;

    queueCompletedChunk();
    std::cout << "\n[SORT DEBUG] final histogram chunk queued"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " | queued_events=" << g_QueuedBuiltEvents.load()
              << std::endl;
    chunkQueue.set_finished();
    std::cout << "\n[SORT DEBUG] histogram queue marked finished"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " | waiting for histogram consumer"
              << std::endl;

    histogramConsumer.join();
    std::cout << "\n[SORT DEBUG] histogram consumer joined"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " buffer=" << g_idx.load() << "/" << g_buffer_size.load()
              << std::endl;
    doneFlag = true;
    monitorThread.join();
    std::cout << "\n[SORT DEBUG] monitor thread joined"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " buffer=" << g_idx.load() << "/" << g_buffer_size.load()
              << std::endl;
    
    if (chunkBuffer != nullptr) {
        delete chunkBuffer;
    }

    if (writeTree && treeOutput.tree != nullptr) {
        std::cout << "\n[SORT DEBUG] starting final tree close"
                  << " | read=" << g_ReadCount.load()
                  << " built=" << g_BuiltCount.load()
                  << " | entries=" << treeOutput.tree->GetEntriesFast()
                  << std::endl;
        WriteAndCloseTreeOutput(treeOutput);
    }

    std::cout << "\n[SORT DEBUG] starting histogram file write"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " | " << histogramOutfilename.Data()
              << std::endl;
    if (!WriteHistogramFile(histograms, histogramOutfilename, true)) {
        return 5;
    }
    std::cout << "\n[SORT DEBUG] histogram file write finished"
              << " | read=" << g_ReadCount.load()
              << " built=" << g_BuiltCount.load()
              << " | " << histogramOutfilename.Data()
              << std::endl;

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
                          Long64_t TS_TOLERANCE,
                          Long64_t maxSortTs)
{
    std::vector<std::unique_ptr<DigitiserBase>> digitisers = BuildDigitiserList(infilename);

    DigitiserBase::SetTsTolerance(TS_TOLERANCE);

    if (!outfilename.Length()) {
        outfilename = infilename + "_events.root";
    }

    EventTreeOutput treeOutput = CreateEventTreeOutput(outfilename);
    if (treeOutput.tree == nullptr) {
        std::cerr << "Could not create output tree file " << outfilename << '\n';
        return;
    }

    ThreadedBinToTree(digitisers, treeOutput.tree, tdiff, CHUNK, BufferSize, nullptr, maxSortTs);
    WriteAndCloseTreeOutput(treeOutput);
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

    ThreadedHistogramCollection histograms;
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
                 Long64_t histChunkEvents,
                 Long64_t maxSortTs)
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
        if (treeOutput.tree == nullptr) {
            std::cerr << "Could not create output tree file " << eventTreeOutfilename << '\n';
            return 5;
        }

        ThreadedBinToTree(digitisers, treeOutput.tree, tdiff, CHUNK, BufferSize, &ADChists, maxSortTs);
        WriteAndCloseTreeOutput(treeOutput);
        WriteAdcHistogramsToFile(eventTreeOutfilename, ADChists);
        return 0;
    }

    return MakeEventTreeAndHistogramsFromBin(digitisers,
                                             histogramOutfilename,
                                             tdiff,
                                             histWorkers,
                                             CHUNK,
                                             BufferSize,
                                             writeTree ? eventTreeOutfilename : "",
                                             histChunkEvents,
                                             maxSortTs);
}
