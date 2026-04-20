#ifndef JAEASortFillHistograms
#define JAEASortFillHistograms

#include <BuiltEvent.h>
#include <ThreadedHistograms.h>
#include <Detectors.h>
#include <DetectorsAdv.h>

struct DetHitScratch {
    std::vector<HPGeHit> hpge;
    std::vector<CdTeHit> cdte;
    std::vector<DetHit> hits;
    S3Det s3;

    void Clear()
    {
        hpge.clear();
        cdte.clear();
        hits.clear();
        s3.Clear();
    }
};

void FillHistograms(HistogramRefs& H, const BuiltEventView& event);

DetHitScratch& DetHitScratchBuffer();

DetHitScratch& BuildDetHitCategories(const BuiltEventView& event);

#endif
