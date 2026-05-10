#include <IO.h>
#include <ThreadedSort.h>
#include <TStopwatch.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <thread>
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
    const bool OnlineSort = gIO->OnlineSort;


    ConfigureS3DetFromIO();

    int HistWorkers = gIO->GetInput("Workers", 4);
    Long64_t TS_Diff = gIO->GetInput("Window", gTS_Diff);
    int ChunkSize = gIO->GetInput("BinChunk", gBinChunkDefaultSize);
    int BufferSize = gIO->GetInput("BuildBuffer", gBuildBuffDefaultSize);
    Long64_t TsTolerance = gIO->GetInput("Tolerance", gTS_TOLERANCE);
    Long64_t HistChunkEvents = gIO->GetInput("HistChunks", gHistChunkDefaultEvents);
    const bool BasicHistograms = gIO->GetBoolInput("BasicHistograms", false);
    const bool HistogramTimers = gIO->GetBoolInput("HistogramTimers", false);

    gHistogramRuntimeOptions.basicHistogramsOnly = BasicHistograms;
    gHistogramRuntimeOptions.enableHistogramTimers = HistogramTimers;
    ResetHistogramRuntimeTimers();

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
    if (gIO->TestInput("HistChunks")) {
        std::cout << "Histogram chunk event target overridden: " << HistChunkEvents << std::endl;
    }
    if (BasicHistograms) {
        std::cout << "Basic histogram mode enabled: detector histograms will not be created or filled" << std::endl;
    }
    if (HistogramTimers) {
        std::cout << "Histogram timers enabled" << std::endl;
    }
    if (OnlineSort && ReadBin) {
        std::cout << "Online sort mode enabled" << std::endl;
        std::cout << "Type finish, done, end, or stop then press Enter when no more .bin files will be written" << std::endl;
    }

    int status = 0;

    TStopwatch timer;
    bool ranSort = false;

    DigitiserBase::SetOnlineMode(OnlineSort);

    std::thread onlineConsoleThread;
    if (OnlineSort && ReadBin) {
        onlineConsoleThread = std::thread([]() {
            std::string command;
            while (!DigitiserBase::IsOnlineRunFinished() && std::getline(std::cin, command)) {
                std::transform(command.begin(), command.end(), command.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (command == "finish" || command == "done" || command == "end" || command == "stop") {
                    std::cout << "[ONLINE] Final run marker received. End-of-data checks released." << std::endl;
                    DigitiserBase::MarkOnlineRunFinished();
                    break;
                }

                if (!command.empty()) {
                    std::cout << "[ONLINE] Unrecognised command '" << command
                              << "'. Use finish, done, end, or stop." << std::endl;
                }
            }
        });
    }

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

    if (OnlineSort && ReadBin) {
        DigitiserBase::MarkOnlineRunFinished();
        if (onlineConsoleThread.joinable()) {
            onlineConsoleThread.detach();
        }
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
