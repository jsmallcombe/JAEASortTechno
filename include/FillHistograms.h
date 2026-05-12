#ifndef JAEASortFillHistograms
#define JAEASortFillHistograms

#include <BuiltEvent.h>
#include <ThreadedHistograms.h>
#include <Detectors.h>
#include <DetectorsAdv.h>

class TGraph;

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

struct HistogramGateRefs {
    const TGraph* invkin = nullptr;
    const TGraph* beta = nullptr;
    const TGraph* betabeam = nullptr;
    double cdteS3up = 100, cdteS3down = -100;
    double hpgeS3up = 100, hpgeS3down = -100;
};

void FillHistograms(HistogramRefs& H, const BuiltEventView& event);

const HistogramGateRefs& HistogramGateRefsBuffer();

DetHitScratch& DetHitScratchBuffer();

DetHitScratch& BuildDetHitCategories(const BuiltEventView& event);

#endif
