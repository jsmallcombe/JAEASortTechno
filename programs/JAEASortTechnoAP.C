#include <IO.h>
#include <ThreadedSort.h>
#include <TStopwatch.h>
#include <iostream>
#include <algorithm>
#include <HistogramRuntime.h>
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
    if (gIO->TestInput("XOffset")) {
        const double xOffset = gIO->GetInput("XOffset", 0);
        CdTeHit::XOffset(xOffset);
        S3Det::fXOffset += xOffset;
        std::cout << "CdTe/S3 X offset applied: " << xOffset << std::endl;
    }
    if (gIO->TestInput("YOffset")) {
        const double yOffset = gIO->GetInput("YOffset", 0);
        CdTeHit::YOffset(yOffset);
        S3Det::fYOffset += yOffset;
        std::cout << "CdTe/S3 Y offset applied: " << yOffset << std::endl;
    }
    if (gIO->TestInput("CdTeZOffset")) {
        const double cdTeZOffset = gIO->GetInput("CdTeZOffset", 0);
        CdTeHit::ZOffset(cdTeZOffset);
        std::cout << "CdTe Z offset applied: " << cdTeZOffset << std::endl;
    }

    int HistWorkers = gIO->GetInput("Workers", 4);
    Long64_t TS_Diff = gIO->GetInput("Window", gTS_Diff*10)/10.;
    int ChunkSize = gIO->GetInput("BinChunk", gBinChunkDefaultSize);
    int BufferSize = gIO->GetInput("BuildBuffer", gBuildBuffDefaultSize);
    Long64_t TsTolerance = gIO->GetInput("Tolerance", gTS_TOLERANCE*10)/10;
    Long64_t HistChunkEvents = gIO->GetInput("HistChunks", gHistChunkDefaultEvents);
    Long64_t MaxSortTs =  gIO->GetInput("MaxTs", -1);
    const bool BasicHistograms = gIO->GetBoolInput("BasicHistograms", false);
    const bool HistogramTimers = gIO->GetBoolInput("HistogramTimers", false);
    std::vector<UShort_t> triggerModules;
    for (double value : gIO->GetInputs("TriggerModule")) {
        if (value < 0) {
            continue;
        }
        triggerModules.push_back(static_cast<UShort_t>(value));
    }
    BuiltEvent::SetTriggerModules(std::move(triggerModules));

    gHistogramRuntimeOptions.basicHistogramsOnly = BasicHistograms;
    gHistogramRuntimeOptions.enableHistogramTimers = HistogramTimers;
    ResetHistogramRuntimeTimers();

    cout<<endl<<"Input summary:"<<endl;
    if (gIO->TestInput("Window")) {
        std::cout << "Build window default overidden: " << TS_Diff*10 <<" ns" << std::endl;
    }
    if (gIO->TestInput("BinChunk")) {
        std::cout << "Chunk size overridden: " << ChunkSize << std::endl;
    }
    if (gIO->TestInput("BuildBuffer")) {
        std::cout << "Build buffer overridden: " << BufferSize << std::endl;
    }
    if (gIO->TestInput("Tolerance")) {
        std::cout << "Timestamp tolerance overridden: " << TsTolerance*10 << std::endl;
    }
    if (gIO->TestInput("HistChunks")) {
        std::cout << "Histogram chunk event target overridden: " << HistChunkEvents << std::endl;
    }
    if (MaxSortTs>-1) {
        std::cout << "Maximum sorting timestamp enabled: " << MaxSortTs << " ticks" << std::endl;
    }
    if (BasicHistograms) {
        std::cout << "Basic histogram mode enabled: detector histograms will not be created or filled" << std::endl;
    }
    if (HistogramTimers) {
        std::cout << "Histogram timers enabled" << std::endl;
    }
    if (BuiltEvent::TriggersEnabled) {
        std::cout << "Trigger modules:";
        for (size_t mod = 0; mod < BuiltEvent::TriggerModules.size(); ++mod) {
            if (BuiltEvent::TriggerModules[mod]) {
                std::cout << ' ' << mod;
            }
        }
        std::cout << std::endl;
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
                              HistChunkEvents,
                              MaxSortTs);
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
        if (HistogramTimers) {
            PrintHistogramRuntimeTimers(std::cout);
            std::cout << std::endl;
        }
    }

    if (EventData != nullptr) delete EventData;
    if (gIO != nullptr) delete gIO;
    return status;
}
