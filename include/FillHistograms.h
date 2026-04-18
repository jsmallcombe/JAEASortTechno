#ifndef JAEASortFillHistograms
#define JAEASortFillHistograms

#include <BuiltEvent.h>
#include <ThreadedHistograms.h>
#include <Detectors.h>

struct DetHitScratch {
    std::vector<DetHit> hpge;
    std::vector<DetHit> laBr;
    std::vector<DetHit> siDeltaE;
    std::vector<DetHit> si;
    std::vector<DetHit> siDeltaE_B;
    std::vector<DetHit> si_B;
    std::vector<DetHit> solar;
    std::vector<DetHit> dice;
    std::vector<DetHit> cdte;
};

void FillHistograms(HistogramRefs& H, const BuiltEventView& event);

DetHitScratch& DetHitScratchBuffer();

DetHitScratch& BuildDetHitCategories(const BuiltEventView& event);

#endif
