#ifndef JAEASortThreadedHistogramCollection
#define JAEASortThreadedHistogramCollection

#include <BuiltEvent.h>
#include <HistogramRuntime.h>
#include <ThreadedHistograms.h>
#include <ThreadedHistogramsBasic.h>

struct HistogramCollectionRefs {
    HistogramRefsBasic Basic;
    HistogramRefs Detector;
};

class ThreadedHistogramCollection {
public:
    ThreadedHistogramSetBasic Basic;
    ThreadedHistogramSet Detector;

    HistogramCollectionRefs ResolveHistogramRefs()
    {
        HistogramCollectionRefs refs;
        refs.Basic = Basic.ResolveHistogramRefs();
        if (!gHistogramRuntimeOptions.basicHistogramsOnly) {
            refs.Detector = Detector.ResolveHistogramRefs();
        }
        return refs;
    }

    void WriteAll(TDirectory* outputDirectory = nullptr)
    {
        Basic.WriteAll(outputDirectory);
        if (!gHistogramRuntimeOptions.basicHistogramsOnly) {
            Detector.WriteAll(outputDirectory);
        }
    }
};

void FillSelectedHistograms(HistogramCollectionRefs& H, const BuiltEventView& event);

#endif
