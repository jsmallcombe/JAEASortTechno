#include <IO.h>
#include <ThreadedSort.h>
#include <TStopwatch.h>
#include <iostream>
#include <IOHelpers.h>

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


    ConfigureS3DetFromIO();

    int HistWorkers = gIO->GetInput("Workers", 4);
    Long64_t TS_Diff = gIO->GetInput("Window", gTS_Diff);
    int ChunkSize = gIO->GetInput("BinChunk", gBinChunkDefaultSize);
    int BufferSize = gIO->GetInput("BuildBuffer", gBuildBuffDefaultSize);
    Long64_t TsTolerance = gIO->GetInput("Tolerance", gTS_TOLERANCE);
    Long64_t HistChunkEvents = gIO->GetInput("HistChunks", gHistChunkDefaultEvents);

    cout<<endl<<"Input summary:"<<endl;
    if (gIO->TestInput("Window")) {
        std::cout << "Build window default overidden: " << TS_Diff <<" ns" << std::endl;
    }
    if (gIO->TestInput("BinChunk")) {
        std::cout << "Chunk size overridden: " << ChunkSize << std::endl;
    }
    if (gIO->TestInput("BuildBuffer")) {
        std::cout << "Build buffer overridden: " << BufferSize << std::endl;
    }
    if (gIO->TestInput("Tolerance")) {
        std::cout << "Timestamp tolerance overridden: " << TsTolerance << std::endl;
    }
    if (gIO->TestInput("HistChunkEvents")) {
        std::cout << "Histogram chunk event target overridden: " << HistChunkEvents << std::endl;
    }

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
                              BufferSize,
                              TsTolerance,
                              HistChunkEvents);
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
