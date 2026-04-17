#ifndef JAEASortFillHistograms
#define JAEASortFillHistograms

#include <BuiltEvent.h>
#include <ThreadQueue.h>
#include <ThreadedHistograms.h>

#include <TString.h>
#include <TTree.h>

enum class EventTreeQueueHistMode {
    PersistentWorkers = 0,
    PerChunkFillHistogramsFromEventTree = 1
};

void FillHistograms(ThreadedHistogramSet& histograms, const BuiltEventView& event);

void FillHistogramsFromEventTree(TTree* tree,
                                 ThreadedHistogramSet& histograms,
                                 unsigned int nthreads = 0);

void FillHistogramsFromBuiltEventQueue(ThreadSafeQueue<BuiltEvent>& queue,
                                       ThreadedHistogramSet& histograms,
                                       unsigned int nthreads = 0);

size_t FillHistogramsFromBuiltEventChunk(BuiltEventChunkBuffer& chunk,
                                         ThreadedHistogramSet& histograms,
                                         unsigned int nthreads = 0);

void FillHistogramsFromBuiltEventChunkQueue(ThreadSafeQueue<BuiltEventChunkBuffer*>& queue,
                                            ThreadedHistogramSet& histograms,
                                            unsigned int nthreads = 0);

void FillHistogramsFromBuiltEventChunkQueueUsingExistingFunction(ThreadSafeQueue<BuiltEventChunkBuffer*>& queue,
                                                                 ThreadedHistogramSet& histograms,
                                                                 unsigned int nthreads = 0);

bool WriteHistogramFile(ThreadedHistogramSet& histograms,
                        const TString& outfilename,
                        bool overwrite = false);

#endif
