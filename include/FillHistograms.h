#ifndef JAEASortFillHistograms
#define JAEASortFillHistograms

#include <BuiltEvent.h>
#include <ThreadedHistograms.h>
#include <Detectors.h>
#include <DetectorsAdv.h>

class TGraph;

#include <TSpline.h>

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
    double gammaoffset = 1;
    bool goff = false;
    bool blur = false;
    TGraph* invkinl = nullptr;
    TGraph* betal = nullptr;
    TGraph* betabeaml = nullptr;
    TSpline3 invkinSp;
    TSpline3 betaSp;
    TSpline3 betabeamSp;
    double cdteS3up = 100, cdteS3down = -100;
    double cdteS3backup = 100, cdteS3backdown = -100;
    double cdteS3tzero = 0, cdtekevns = -0.5,  cdteezero = 120;
    double hpgeS3up = 100, hpgeS3down = -100;
    double hpgeS3backup = 100, hpgeS3backdown = -100;
    double cdtecdtegate = 100;
    double cdtehpgeup = 100, cdtehpgedown = -100;
    double hpgehpgegate = 100;

    double pixelcut = 5;
};

void FillHistograms(HistogramRefs& H, const BuiltEventView& event);

const HistogramGateRefs& HistogramGateRefsBuffer();

DetHitScratch& DetHitScratchBuffer();

DetHitScratch& BuildDetHitCategories(const BuiltEventView& event);

#endif
