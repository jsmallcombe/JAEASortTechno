#ifndef JAEASortThreadedHistFill
#define JAEASortThreadedHistFill

#include <FillHistograms.h>
#include <ThreadQueue.h>

#include <TString.h>
#include <TTree.h>

// Histogram sort for an existing built-event tree or chain.
// Uses ROOT's tree readers, optionally with TTreeProcessorMT,
// and updates a single shared ThreadedHistogramSet.
void FillHistogramsFromEventTree(TTree* tree,
                                 ThreadedHistogramSet& histograms,
                                 unsigned int nthreads = 0);

// Consume completed built-event chunks from the bounded queue and
// process them with a persistent pool of histogram workers until
// the producer marks the queue finished.
void FillHistogramsFromBuiltEventChunkQueue(ThreadSafeQueue<BuiltEventChunkBuffer*>& queue,
                                            ThreadedHistogramSet& histograms,
                                            unsigned int nthreads = 0);

// Write the merged histogram set to the requested ROOT file.
// This is shared by both the direct raw-bin path and the
// existing event-tree histogram path.
bool WriteHistogramFile(ThreadedHistogramSet& histograms,
                        const TString& outfilename,
                        bool overwrite = false);

#endif
