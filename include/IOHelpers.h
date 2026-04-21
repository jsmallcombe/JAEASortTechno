#ifndef JAEASortIOHelpers
#define JAEASortIOHelpers

#include <IO.h>
#include <DetectorsAdv.h>

void ConfigureS3DetFromIO()
{
    S3Det::fFrontBackEnergy = gIO->GetInput("S3FrontBackEnergy", S3Det::fFrontBackEnergy);
    S3Det::fFrontBackOffset = gIO->GetInput("S3FrontBackOffset", S3Det::fFrontBackOffset);
    S3Det::fFrontBackTime = gIO->GetInput("S3FrontBackTime", S3Det::fFrontBackTime);

    S3Det::fPreferSector = gIO->GetBoolInput("S3PreferenceSector", S3Det::fPreferSector);
    S3Det::fAllowMultiHit = gIO->GetBoolInput("S3MultiHit", S3Det::fAllowMultiHit);
    S3Det::fKeepShared = gIO->GetBoolInput("S3KeepShared", S3Det::fKeepShared);
    S3Det::fFlipPhi = gIO->GetBoolInput("S3FlipPhi", S3Det::fFlipPhi);
    S3Det::fTargetDistance = gIO->GetInput("S3TargetDistance", S3Det::fTargetDistance);

    if (gIO->TestInput("S3OffsetPhiSetDeg")) {
        S3Det::fOffsetPhiSet = gIO->GetInput("S3OffsetPhiSetDeg") * 3.14159265358979323846 / 180.0;
    }

    std::cout << "S3 settings: FrontBackEnergy=" << S3Det::fFrontBackEnergy
              << " FrontBackOffset=" << S3Det::fFrontBackOffset
              << " FrontBackTime=" << S3Det::fFrontBackTime
              << " PreferenceSector=" << S3Det::fPreferSector
              << " MultiHit=" << S3Det::fAllowMultiHit
              << " KeepShared=" << S3Det::fKeepShared
              << " FlipPhi=" << S3Det::fFlipPhi
              << " PhiOffset=" << S3Det::fOffsetPhiSet
              << " TargetDistance=" << S3Det::fTargetDistance
              << std::endl;
}

#endif
