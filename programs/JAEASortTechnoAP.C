#include <IO.h>
#include <ThreadedSort.h>

#include <TStopwatch.h>

#include <iostream>

int main(int argc, char** argv)
{
    gIO = new JAEASortIO(argc, argv);
    if (gIO == nullptr) return 1;
    if (!gIO->Validated) return 2;

    bool ReadBin = (gIO->Digitisers.size() > 0);
    TChain* EventData = gIO->DataTree("EventTree");
    bool ReadTree = (EventData != nullptr);
    if (ReadTree) ReadBin = false;

    bool WriteTree = gIO->WriteEventTree;
    bool DoHistSort = gIO->DoHistSort;
    bool Overwrite = gIO->Overwrite;

    cout<<endl<<"Input summary:"<<endl;
    int HistWorkers = gIO->GetInput("Workers", 4);
    Long64_t TS_Diff = gIO->GetInput("Window", gTS_Diff);
    int ChunkSize = gIO->GetInput("Chunk", gBinChunkDefaultSize);
    int QueueSize = gIO->GetInput("Queue", gThreadQueueBuiltEvents);
    int BufferSize = gIO->GetInput("Buffer", gBuildBuffDefaultSize);
    Long64_t TsTolerance = gIO->GetInput("Tolerance", gTS_TOLERANCE);
    Long64_t HistChunkEvents = gIO->GetInput("HistChunkEvents", 100000);
    int HistTreeModeInput = gIO->GetInput("HistTreeMode", 0);
    EventTreeQueueHistMode HistTreeMode =
        (HistTreeModeInput == 1)
            ? EventTreeQueueHistMode::PerChunkFillHistogramsFromEventTree
            : EventTreeQueueHistMode::PersistentWorkers;

    if (gIO->TestInput("Window")) {
        std::cout << "Build window default overidden: " << TS_Diff << std::endl;
    }
    if (gIO->TestInput("Chunk")) {
        std::cout << "Chunk size overridden: " << ChunkSize << std::endl;
    }
    if (gIO->TestInput("Queue")) {
        std::cout << "Built-event queue budget overridden: " << QueueSize << std::endl;
    }
    if (gIO->TestInput("Buffer")) {
        std::cout << "Build buffer overridden: " << BufferSize << std::endl;
    }
    if (gIO->TestInput("Tolerance")) {
        std::cout << "Timestamp tolerance overridden: " << TsTolerance << std::endl;
    }
    if (gIO->TestInput("HistChunkEvents")) {
        std::cout << "Histogram chunk event target overridden: " << HistChunkEvents << std::endl;
    }
    if (gIO->TestInput("HistTreeMode")) {
        std::cout << "Histogram tree consumer mode overridden: " << HistTreeModeInput << std::endl;
    }

    cout<<endl<<gIO->TestInput("Buffer")<<endl;
    cout<<"Build window: "<<TS_Diff<<endl;
    cout<<"Chunk size: "<<ChunkSize<<endl;
    cout<<"Built-event queue budget: "<<QueueSize<<endl;
    cout<<"Build buffer: "<<BufferSize<<endl;
    cout<<"Timestamp tolerance: "<<TsTolerance<<endl;
    cout<<"Histogram chunk event target: "<<HistChunkEvents<<endl;
    cout<<"Histogram tree consumer mode: "<<HistTreeModeInput<<endl;
    int status = 0;

    TStopwatch timer;
    bool ranSort = false;

    if (ReadBin) {
        timer.Start();
        ranSort = true;
        status = ThreadedSort(gIO->Digitisers,
                              gIO->EventTreeOutFilename,
                              gIO->HistogramOutFilename,
                              TS_Diff,
                              Overwrite,
                              WriteTree,
                              DoHistSort,
                              HistWorkers,
                              ChunkSize,
                              QueueSize,
                              BufferSize,
                              TsTolerance,
                              HistChunkEvents,
                              HistTreeMode);
    } else if (ReadTree) {
        timer.Start();
        ranSort = true;
        status = ThreadedSort(EventData,
                              gIO->HistogramOutFilename,
                              HistWorkers,
                              Overwrite);
    }

    if (ranSort) {
        timer.Stop();
        std::cout << "\nDone\n";
        std::cout << Form("\n RealTime = %d seconds, CpuTime = %d seconds\n\n",
                          static_cast<Int_t>(timer.RealTime()),
                          static_cast<Int_t>(timer.CpuTime()));
    }

    if (EventData != nullptr) delete EventData;
    if (gIO != nullptr) delete gIO;
    return status;
}
