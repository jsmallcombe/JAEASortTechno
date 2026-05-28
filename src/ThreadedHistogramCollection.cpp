#include <ThreadedHistogramCollection.h>

#include <FillHistograms.h>
#include <FillHistogramsBasic.h>

void FillSelectedHistograms(HistogramCollectionRefs& H, const BuiltEventView& event)
{
    FillHistogramsBasic(H.Basic, event);
    if (!gHistogramRuntimeOptions.basicHistogramsOnly) {
        FillHistograms(H.Detector, event);
    }
}
