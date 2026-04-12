#ifndef hThreadFunctions
#define hThreadFunctions

#include <TFile.h>
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


void BinToThreadedQueue(ThreadSafeQueue<std::vector<Event>>& queue,
              std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
              size_t CHUNK_SIZE);


void ThreadedQueueToEventTree(ThreadSafeQueue<std::vector<Event>>& queue,
              TTree* outtree,
              Long64_t tdiff,
              size_t BUFFER);

void ThreadedQueueToBuiltEventQueue(ThreadSafeQueue<std::vector<Event>>& input_queue,
              ThreadSafeQueue<BuiltEvent>& output_queue,
              Long64_t tdiff,
              size_t BUFFER);

void ThreadedQueueToEventTreeAndBuiltEventQueue(ThreadSafeQueue<std::vector<Event>>& input_queue,
              TTree* outtree,
              ThreadSafeQueue<BuiltEvent>& output_queue,
              Long64_t tdiff,
              size_t BUFFER);

int ThreadSort(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                    TTree* outtree,
                    Long64_t tdiff=gTS_Diff,
                    int CHUNK = gBinChunkDefaultSize,
                    int QueueSize = gThreadQueueChunks,
                    int BufferSize=gBuildBuffDefaultSize);


void MakeEventTreeFromBin(TString infilename,
                        TString outfilename="",
                        Long64_t tdiff=gTS_Diff,
                        int CHUNK = gBinChunkDefaultSize,
                        int QueueSize = gThreadQueueChunks,
                        int BufferSize=gBuildBuffDefaultSize,
                        Long64_t TS_TOLERANCE = gTS_TOLERANCE);

    


#endif
