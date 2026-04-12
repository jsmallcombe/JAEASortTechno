#ifndef JAEASortFillHistograms
#define JAEASortFillHistograms

#include <BuiltEvent.h>
#include <ThreadQueue.h>
#include <ThreadedHistograms.h>

#include <TTree.h>

void FillHistograms(ThreadedHistogramSet& histograms, const BuiltEventView& event);

void FillHistogramsFromEventTree(TTree* tree,
                                 ThreadedHistogramSet& histograms,
                                 unsigned int nthreads = 0);

void FillHistogramsFromBuiltEventQueue(ThreadSafeQueue<BuiltEvent>& queue,
                                       ThreadedHistogramSet& histograms,
                                       unsigned int nthreads = 0);

#endif
