#include <DetectorsAdv.h>
#include <Globals.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
}

double DopplerCorrectEnergy(double energyKeV, double angleRad, double beta)
{
    if (beta <= 0.0 || beta >= 1.0) {
        return energyKeV;
    }

    const double gamma = 1.0 / std::sqrt(1.0 - beta * beta);
    return energyKeV * gamma * (1.0 - beta * std::cos(angleRad));
}

bool S3Det::fPreferSector = false;
bool S3Det::fAllowMultiHit = false;
bool S3Det::fKeepShared = true;
bool S3Det::fFlipPhi = false;

double S3Det::fOffsetPhiCon = 0.5 * kPi;
double S3Det::fOffsetPhiSet = -22.5 * kPi / 180.0;
double S3Det::fOuterDiameter = 70.0;
double S3Det::fInnerDiameter = 22.0;
double S3Det::fTargetDistance = -30.0;
XYZVector S3Det::fOffset(0.0, 0.0, 0.0);
bool S3Det::fPositionsBuilt = false;
std::array<std::array<XYZVector, S3Det::fSectorNumber>, S3Det::fRingNumber> S3Det::fPixelPositions;
std::array<double, S3Det::fRingNumber> S3Det::fRingRadii;
std::array<double, S3Det::fSectorNumber> S3Det::fSectorPhis;
double S3Det::fRingWidth = 0.0;
double S3Det::fPhiWidth = 0.0;
double S3Det::fBlurSep = 0.0;

double S3Det::fFrontBackTime = 30.0;
double S3Det::fFrontBackEnergy = 0.95;
double S3Det::fFrontBackOffset = 150.0;

XYZVector DetPos::Pos(bool smear) const
{
    if(!fPosSet) {
        BuildPos();
        fPosSet = true;
    }
    return smear ? fBlurPos : fPos;
}

void DetPos::ResetPos() const
{
    fPosSet = false;
    fPos = XYZVector(0.0, 0.0, 0.0);
    fBlurPos = XYZVector(0.0, 0.0, 0.0);
}

void S3Hit::BuildPos() const
{
    if(fRingHit == nullptr || fSectorHit == nullptr) {
        return;
    }

    fBlurPos = S3Det::GetPosition(Ring(), Sector(), true, &fPos);
}

void S3Det::Clear()
{
    fRingHits.clear();
    fSectorHits.clear();
    fHits.clear();
    fPixelsBuilt = false;
}

void S3Det::AddHit(const DetHit& hit)
{
    if(hit.Type() == DetHit::S3Ring) {
        AddRingHit(hit);
    } else if(hit.Type() == DetHit::S3Sector) {
        AddSectorHit(hit);
    }
}

void S3Det::AddHits(const std::vector<DetHit>& hits)
{
    for(const auto& hit : hits) {
        AddHit(hit);
    }
}

void S3Det::AddHit(Double_t tTS, UShort_t tC, UShort_t tMod, UShort_t tChan)
{
    AddHit(DetHit(tTS, tC, tMod, tChan));
}

void S3Det::AddHit(const BuiltEventView& event, std::size_t i)
{
    if(i >= event.Size()) {
        return;
    }

    AddHit(event.Ts[i], event.Adc[i], event.Mod[i], event.Ch[i]);
}

void S3Det::AddRingHit(const DetHit& hit)
{
    fRingHits.push_back(hit);
    fPixelsBuilt = false;
}

void S3Det::AddRingHit(Double_t tTS, UShort_t tC, UShort_t tMod, UShort_t tChan)
{
    AddRingHit(DetHit(tTS, tC, tMod, tChan));
}

void S3Det::AddRingHit(const BuiltEventView& event, std::size_t i)
{
    if(i >= event.Size()) {
        return;
    }

    AddRingHit(event.Ts[i], event.Adc[i], event.Mod[i], event.Ch[i]);
}

void S3Det::AddSectorHit(const DetHit& hit)
{
    fSectorHits.push_back(hit);
    fPixelsBuilt = false;
}

void S3Det::AddSectorHit(Double_t tTS, UShort_t tC, UShort_t tMod, UShort_t tChan)
{
    AddSectorHit(DetHit(tTS, tC, tMod, tChan));
}

void S3Det::AddSectorHit(const BuiltEventView& event, std::size_t i)
{
    if(i >= event.Size()) {
        return;
    }

    AddSectorHit(event.Ts[i], event.Adc[i], event.Mod[i], event.Ch[i]);
}

void S3Det::ResetRingsSectors()
{
    fRingHits.clear();
    fSectorHits.clear();
    fHits.clear();
    fPixelsBuilt = false;
}

std::size_t S3Det::GetPixelMultiplicity()
{
    BuildHits();
    return fHits.size();
}

const std::vector<S3Hit>& S3Det::Hits()
{
    BuildHits();
    return fHits;
}

const S3Hit* S3Det::GetS3Hit(std::size_t i)
{
    BuildHits();
    return i < fHits.size() ? &fHits[i] : nullptr;
}

const DetHit* S3Det::GetRingHit(std::size_t i) const
{
    return i < fRingHits.size() ? &fRingHits[i] : nullptr;
}

const DetHit* S3Det::GetSectorHit(std::size_t i) const
{
    return i < fSectorHits.size() ? &fSectorHits[i] : nullptr;
}

bool S3Det::TimeMatches(const DetHit& ring, const DetHit& sector) const
{
    return std::abs(ring.Time() - sector.Time()) <= fFrontBackTime;
}

bool S3Det::EnergyMatches(double ringEnergy, double sectorEnergy) const
{
    return (ringEnergy - fFrontBackOffset) * fFrontBackEnergy < sectorEnergy &&
           (sectorEnergy - fFrontBackOffset) * fFrontBackEnergy < ringEnergy;
   // return (double)ringEnergy/(double)sectorEnergy > 0.8 || (double)ringEnergy/(double)sectorEnergy < 1.2;
}

void S3Det::AddPixel(const DetHit* ring, const DetHit* sector, const DetHit* primary)
{
    if(ring == nullptr || sector == nullptr) {
        return;
    }

    const DetHit* resolvedPrimary = primary != nullptr ? primary : (fPreferSector ? sector : ring);
    fHits.emplace_back(ring, sector, resolvedPrimary);
}

void S3Det::BuildHits()
{
    // Constructs the front/back coincidences to create pixels based on energy and time differences.
    // Energy and time differences can be changed using the SetFrontBackEnergy and SetFrontBackTime functions.
    // Shared rings and sectors can be constructed, by default they are not kept unless requested.
    // To enable multi-hit reconstruction, use SetMultiHit.

    // If the pixels have been reset (or never set), clear the pixel hits first.
    //if(fPixelsBuilt) {
    //    return;
    //}

    fHits.clear();

    if(fRingHits.empty() || fSectorHits.empty()) {
        fPixelsBuilt = true;
        return;
    }

    // We are going to want energies several times, so build quick lookup vectors.
    std::vector<double> ringEnergy;
    std::vector<double> sectorEnergy;
    std::vector<bool> usedRing(fRingHits.size(), false);
    std::vector<bool> usedSector(fSectorHits.size(), false);

    ringEnergy.reserve(fRingHits.size());
    sectorEnergy.reserve(fSectorHits.size());

    for(auto& hit : fRingHits) {
        ringEnergy.push_back(hit.Energy());
    }
    for(auto& hit : fSectorHits) {
        sectorEnergy.push_back(hit.Energy());
    }

    // Loop over both vectors and build energy+time matching hits.
    for(std::size_t i = 0; i < fRingHits.size(); ++i) {
        for(std::size_t j = 0; j < fSectorHits.size(); ++j) {
            if(TimeMatches(fRingHits[i], fSectorHits[j]) && EnergyMatches(ringEnergy[i], sectorEnergy[j])) {
                AddPixel(&fRingHits[i], &fSectorHits[j]);
                usedRing[i] = true;
                usedSector[j] = true;
            }
        }
    }

    if(fAllowMultiHit||fKeepShared) {
        // Shared ring loop.
        for(std::size_t i = 0; i < fRingHits.size(); ++i) {
            if(usedRing[i]) {
                continue;
            }

            for(std::size_t j = 0; j < fSectorHits.size(); ++j) {
                if(usedSector[j]) {
                    continue;
                }

                for(std::size_t k = j + 1; k < fSectorHits.size(); ++k) {
                    if(usedSector[k]) {
                        continue;
                    }

                    if(!TimeMatches(fRingHits[i], fSectorHits[j]) || !TimeMatches(fRingHits[i], fSectorHits[k])) {
                        continue;
                    }

                    if(!EnergyMatches(ringEnergy[i], sectorEnergy[j] + sectorEnergy[k])) {
                        continue;
                    }

                    const int sectorSep = std::abs(static_cast<int>(fSectorHits[j].Index()) - static_cast<int>(fSectorHits[k].Index()));
                    // const bool adjacentSector = sectorSep == 1 || sectorSep == (fSectorNumber - 1);
                    const bool adjacentSector = true;//bug hunting

                    if(adjacentSector) {
                        // Same ring and neighbour sectors, almost certainly charge sharing. Ring becomes primary.
                        if(fKeepShared) {
                            const DetHit* dominantSector = sectorEnergy[j] >= sectorEnergy[k] ? &fSectorHits[j] : &fSectorHits[k];
                            AddPixel(&fRingHits[i], dominantSector, &fRingHits[i]);
                        }
                    } else {
                        if(fAllowMultiHit) {
                        // Two separate hits with a shared ring: each primary is sector.
                            AddPixel(&fRingHits[i], &fSectorHits[j], &fSectorHits[j]);
                            AddPixel(&fRingHits[i], &fSectorHits[k], &fSectorHits[k]);
                        }
                    }

                    usedRing[i] = true;
                    usedSector[j] = true;
                    usedSector[k] = true;
                }
            }
        }

        // Shared sector loop.
        for(std::size_t i = 0; i < fSectorHits.size(); ++i) {
            if(usedSector[i]) {
                continue;
            }

            for(std::size_t j = 0; j < fRingHits.size(); ++j) {
                if(usedRing[j]) {
                    continue;
                }

                for(std::size_t k = j + 1; k < fRingHits.size(); ++k) {
                    if(usedRing[k]) {
                        continue;
                    }

                    if(!TimeMatches(fRingHits[j], fSectorHits[i]) || !TimeMatches(fRingHits[k], fSectorHits[i])) {
                        continue;
                    }

                    if(!EnergyMatches(ringEnergy[j] + ringEnergy[k], sectorEnergy[i])) {
                        continue;
                    }

                    const int ringSep = std::abs(static_cast<int>(fRingHits[j].Index()) - static_cast<int>(fRingHits[k].Index()));
                    // const bool adjacentRing = ringSep == 1;
                    const bool adjacentRing = true;//bug hunting
                    if(adjacentRing) {
                        // Same sector and neighbour rings, almost certainly charge sharing.
                        if(fKeepShared) {
                            const DetHit* dominantRing = ringEnergy[j] >= ringEnergy[k] ? &fRingHits[j] : &fRingHits[k];
                            AddPixel(dominantRing, &fSectorHits[i], &fSectorHits[i]);
                        }
                    } else {
                        if(fAllowMultiHit) {
                            // Two separate hits with a shared sector: the sector is the single side.
                            AddPixel(&fRingHits[j], &fSectorHits[i], &fRingHits[j]);
                            AddPixel(&fRingHits[k], &fSectorHits[i], &fRingHits[k]);
                        }
                    }

                    usedSector[i] = true;
                    usedRing[j] = true;
                    usedRing[k] = true;
                }
            }
        }
    }

    fPixelsBuilt = true;
}

void S3Det::BuildPositions()
{
    // S3 ring and sector indices are treated as 0-based here: ring 0 is the inner ring,
    // and sector 0 starts at phi=0 before the fixed connector/setup offsets.
    fRingWidth = (fOuterDiameter - fInnerDiameter) * 0.5 / static_cast<double>(fRingNumber);
    const double innerRadius = fInnerDiameter * 0.5;
    fPhiWidth = 2.0 * kPi / static_cast<double>(fSectorNumber);
    fBlurSep = fRingWidth * 0.025;

    for(unsigned int sector = 0; sector < fSectorNumber; sector++) {
        double phi = fPhiWidth * static_cast<double>(sector) + fOffsetPhiCon;
        if(fFlipPhi) {
            phi = -phi;
        }
        fSectorPhis[sector] = phi + fOffsetPhiSet;
    }

    for(unsigned int ring = 0; ring < fRingNumber; ring++) {
        const double radius = innerRadius + fRingWidth * (static_cast<double>(ring) + 0.5);
        fRingRadii[ring] = radius;

        for(unsigned int sector = 0; sector < fSectorNumber; sector++) {
            const double phi = fSectorPhis[sector];
            fPixelPositions[ring][sector] = XYZVector(std::cos(phi) * radius + fOffset.X(),
                                                       std::sin(phi) * radius + fOffset.Y(),
                                                       fTargetDistance + fOffset.Z());
        }
    }

    fRingWidth-=fBlurSep;
    fRingWidth*=0.5;
    fPositionsBuilt = true;
}

XYZVector S3Det::GetPosition(unsigned int ring, unsigned int sector, bool smear, XYZVector* pos)
{
    if(!fPositionsBuilt) {
        BuildPositions();
    }

    ring %= fRingNumber;
    sector %= fSectorNumber;

    const XYZVector& basePos = fPixelPositions[ring][sector];
    if(pos != nullptr) {*pos = basePos;}
    if(!smear) return basePos;

    const double radius = fRingRadii[ring] + gThRand().Uniform(-fRingWidth,fRingWidth);
    const double phiSep = fBlurSep / radius;
    const double phi = gThRand().Uniform(fSectorPhis[sector] - 0.5 * fPhiWidth + phiSep,
                                         fSectorPhis[sector] + 0.5 * fPhiWidth - phiSep);
    const double dx = std::cos(phi) * radius - std::cos(fSectorPhis[sector]) * fRingRadii[ring];
    const double dy = std::sin(phi) * radius - std::sin(fSectorPhis[sector]) * fRingRadii[ring];
    // We havent actually saved much computation by precalculating, as blue is calculated and stored by DetPos child 
    // even if it isnt used, and we have to calculate the raw vec here (Or store vec with and without offset)

    return XYZVector(basePos.X() + dx, basePos.Y() + dy, basePos.Z());
}

void S3Det::SetOffset(const XYZVector& offset)
{
    fOffset = offset;
    fPositionsBuilt = false;
}

XYZVector CdTeWorldVectors[16]{
    {3.5, -25.71, 15.806},
    {-3.5, -25.71, 15.806},
    {-3.5, -28.668, 9.462},
    {3.5, -28.668, 9.462},
    {3.5, -14.34, 26.537},
    {-3.5, -14.34, 26.537},
    {-3.5, -20.074, 22.522},
    {3.5, -20.074, 22.522},
    {3.5, 20.074, 22.522},
    {-3.5, 20.074, 22.522},
    {-3.5, 14.34, 26.537},
    {3.5, 14.34, 26.537},
    {3.5, 28.668, 9.462},
    {-3.5, 28.668, 9.462},
    {-3.5, 25.71, 15.806},
    {3.5, 25.71, 15.806}
};
double CdTeZYsmear[4][2]{
    {0.845,1.813},
    {1.6385,1.147},
    {1.6385,-1.147},
    {0.845,-1.813}
};

void CdTeHit::BuildPos() const
{
    fBlurPos = PosStatic(true, Index(), &fPos);
}

UShort_t CdTeHit::DetectorNumber() const
{
    return Index() / 4;
}

UShort_t CdTeHit::DetectorPixel() const
{
    return Index() % 4;
}

bool CdTeHit::IsNeighbour(const CdTeHit& other) const
{
    if(DetectorNumber() != other.DetectorNumber()) {
        return false;
    }

    const int pixelDiff = std::abs(static_cast<int>(DetectorPixel()) - static_cast<int>(other.DetectorPixel()));
    return pixelDiff == 1 || pixelDiff == 3;
}

Double_t CdTeHit::DopplerCorrectedEnergy(double angleRad, double beta) const
{
    return DopplerCorrectEnergy(Energy(), angleRad, beta);
}

void CdTeHit::SetOffset(const XYZVector& offset)
{
    for(auto& vector : CdTeWorldVectors) {
        vector = vector + offset;
    }
}

XYZVector CdTeHit::PosStatic(bool smear,u_short i, XYZVector* pos) 
{
    if(i < 16) {
        if(pos != nullptr) {
            *pos = CdTeWorldVectors[i];
        }
        if(!smear)return CdTeWorldVectors[i];
        double zyr=gThRand().Uniform(-1,1);
        XYZVector smearvec(gThRand().Uniform(-2,2),CdTeZYsmear[i/4][0]*zyr,CdTeZYsmear[i/4][1]*zyr);
        return CdTeWorldVectors[i]+smearvec;
    }
    return XYZVector(0.0, 0.0, 0.0);
}

XYZVector HPGeWorldVectors[6]{
    {-78.89,54.293,-25.882},
    {-90,0,0},
    {-78.89,-54.293,-25.882},
    {78.89,-54.293,-25.882},
    {90,0,0},
    {78.89,54.293,-25.882}
};

XYZVector HPGeSmearBasis[6][2]{
    {{0.566926716, 0.823768231, 0.0}, {-0.214920560, 0.147910787, 0.965366020}},
    {{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}},
    {{-0.566926716, 0.823768231, 0.0}, {-0.214920560, -0.147910787, 0.965366020}},
    {{-0.566926716, -0.823768231, 0.0}, {0.214920560, -0.147910787, 0.965366020}},
    {{0.0, 1.0, 0.0}, {0.0, 0.0, -1.0}},
    {{0.566926716, -0.823768231, 0.0}, {0.214920560, 0.147910787, 0.965366020}}
};

void HPGeHit::BuildPos() const
{
    fBlurPos = PosStatic(true, Index(), &fPos);
}

Double_t HPGeHit::DopplerCorrectedEnergy(double angleRad, double beta) const
{
    return DopplerCorrectEnergy(Energy(), angleRad, beta);
}

XYZVector HPGeHit::PosStatic(bool smear,u_short i, XYZVector* pos) 
{
    if(i < 6) {
        if(pos != nullptr) {*pos = HPGeWorldVectors[i];}
        if(!smear)return HPGeWorldVectors[i];

        const double phi = gThRand().Uniform(0.0, 2.0 * kPi);
        const double length = std::sqrt(gThRand().Uniform(0.0, 30.0 * 30.0));
        const XYZVector smearvec =
            (std::cos(phi) * length) * HPGeSmearBasis[i][0] +
            (std::sin(phi) * length) * HPGeSmearBasis[i][1];

        return HPGeWorldVectors[i] + smearvec;
    }
    return XYZVector(0.0, 0.0, 0.0);
}
