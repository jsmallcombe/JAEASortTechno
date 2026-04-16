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

#include <ThreadQueue.h>
#include <Globals.h>
#include <BuiltEvent.h>
#include <Digitisers.h>

// Producer stage for the threaded bin pipeline.
// Reads chunked raw events from the active digitisers and pushes them into
// the shared raw-event queue until all digitiser inputs are exhausted.
void BinToThreadedQueue(ThreadSafeQueue<std::vector<Event>>& queue,
              std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
              size_t CHUNK_SIZE);

// Consumer stage that builds coincidence events and writes them to a TTree.
// Input is the raw-event queue from BinToThreadedQueue; output is only the
// event tree, with no histogram side path.
void ThreadedQueueToEventTree(ThreadSafeQueue<std::vector<Event>>& queue,
              TTree* outtree,
              Long64_t tdiff,
              size_t BUFFER);

// Consumer stage that builds coincidence events and forwards them onward.
// This version does not write a tree; it converts raw queued events into
// BuiltEvent objects for later histogram filling.
void ThreadedQueueToBuiltEventQueue(ThreadSafeQueue<std::vector<Event>>& input_queue,
              ThreadSafeQueue<BuiltEvent>& output_queue,
              Long64_t tdiff,
              size_t BUFFER);

// Combined consumer stage for the one-pass tree+histogram workflow.
// Each built event is written to the output TTree and also copied into the
// BuiltEvent queue so histogram workers can process the same stream.
void ThreadedQueueToEventTreeAndBuiltEventQueue(ThreadSafeQueue<std::vector<Event>>& input_queue,
              TTree* outtree,
              ThreadSafeQueue<BuiltEvent>& output_queue,
              Long64_t tdiff,
              size_t BUFFER);

// Low-level threaded bin-to-tree driver.
// Starts the producer and tree-writing consumer threads around an existing
// digitiser list and an already-created output TTree.
int ThreadedBinToTree(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                    TTree* outtree,
                    Long64_t tdiff=gTS_Diff,
                    int CHUNK = gBinChunkDefaultSize,
                    int QueueSize = gThreadQueueChunks,
                    int BufferSize=gBuildBuffDefaultSize);

// Convenience entry point for building only an EventTree from a bin stem.
// This function creates the digitiser list and output ROOT objects itself,
// then runs the threaded bin-to-tree pipeline with the requested settings.
void MakeEventTreeFromBin(TString infilename,
                        TString outfilename="",
                        Long64_t tdiff=gTS_Diff,
                        int CHUNK = gBinChunkDefaultSize,
                        int QueueSize = gThreadQueueChunks,
                        int BufferSize=gBuildBuffDefaultSize,
                        Long64_t TS_TOLERANCE = gTS_TOLERANCE);

// Convenience entry point for the combined bin-to-tree and/or histogram path.
// Uses an existing digitiser list and runs the full threaded queue pipeline,
// optionally producing an EventTree alongside the histogram output.
int MakeEventTreeAndHistogramsFromBin(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                                      TString histogramOutfilename,
                                      Long64_t tdiff = gTS_Diff,
                                      unsigned int histWorkers = 0,
                                      int CHUNK = gBinChunkDefaultSize,
                                      int QueueSize = gThreadQueueChunks,
                                      int BufferSize = gBuildBuffDefaultSize,
                                      Long64_t TS_TOLERANCE = gTS_TOLERANCE,
                                      TString treeOutfilename = "");

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
                 int QueueSize = gThreadQueueChunks,
                 int BufferSize = gBuildBuffDefaultSize,
                 Long64_t TS_TOLERANCE = gTS_TOLERANCE);

    


#endif
