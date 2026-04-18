#ifndef hThreadFunctions
#define hThreadFunctions

#include <TFile.h>
#include <TChain.h>
#include <TTree.h>
#include <TString.h>
#include <TStopwatch.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>

#include <ThreadedHistFill.h>
#include <ThreadQueue.h>
#include <Globals.h>
#include <BuiltEvent.h>
#include <Digitisers.h>

// Low-level threaded bin-to-tree driver.
// Starts the producer and tree-writing consumer threads around an existing
// digitiser list and an already-created output TTree.
int ThreadedBinToTree(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                    TTree* outtree,
                    Long64_t tdiff=gTS_Diff,
                    int CHUNK = gBinChunkDefaultSize,
                    int QueueSize = gThreadQueueBuiltEvents,
                    int BufferSize=gBuildBuffDefaultSize);

// Convenience entry point for building only an EventTree from a bin stem.
// This function creates the digitiser list and output ROOT objects itself,
// then runs the threaded bin-to-tree pipeline with the requested settings.
void MakeEventTreeFromBin(TString infilename,
                        TString outfilename="",
                        Long64_t tdiff=gTS_Diff,
                        int CHUNK = gBinChunkDefaultSize,
                        int QueueSize = gThreadQueueBuiltEvents,
                        int BufferSize=gBuildBuffDefaultSize,
                        Long64_t TS_TOLERANCE = gTS_TOLERANCE);

// Convenience entry point for the combined bin-to-tree and/or histogram path.
// Uses an existing digitiser list and runs the direct raw-bin event builder,
// always writing the EventTree while optionally handing completed built-event
// chunks to histogram workers for concurrent histogram sorting.
int MakeEventTreeAndHistogramsFromBin(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                                      TString histogramOutfilename,
                                      Long64_t tdiff = gTS_Diff,
                                      unsigned int histWorkers = 0,
                                      int CHUNK = gBinChunkDefaultSize,
                                      int QueueSize = gThreadQueueBuiltEvents,
                                      int BufferSize = gBuildBuffDefaultSize,
                                      Long64_t TS_TOLERANCE = gTS_TOLERANCE,
                                      TString treeOutfilename = "",
                                      Long64_t histChunkEvents = 100000,
                                      EventTreeQueueHistMode histTreeMode = EventTreeQueueHistMode::PersistentWorkers);

// High-level threaded histogram sort for an existing EventTree/TChain input.
// Reads built events from ROOT rather than raw bin data and writes only the
// histogram file using the requested number of histogram worker threads.
int ThreadedSort(TChain* eventData,
                 TString histogramOutfilename,
                 unsigned int histWorkers = 0,
                 bool overwrite = false);

// High-level threaded sort entry point for raw digitiser input.
// Selects between tree-only, histogram-only, or combined outputs while also
// handling overwrite checks and the relevant queue/buffer configuration.
int ThreadedSort(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                 TString eventTreeOutfilename,
                 TString histogramOutfilename,
                 Long64_t tdiff = gTS_Diff,
                 bool overwrite = false,
                 bool writeTree = true,
                 bool doHistSort = false,
                 unsigned int histWorkers = 0,
                 int CHUNK = gBinChunkDefaultSize,
                 int QueueSize = gThreadQueueBuiltEvents,
                 int BufferSize = gBuildBuffDefaultSize,
                 Long64_t TS_TOLERANCE = gTS_TOLERANCE,
                 Long64_t histChunkEvents = 100000,
                 EventTreeQueueHistMode histTreeMode = EventTreeQueueHistMode::PersistentWorkers);

    


#endif
