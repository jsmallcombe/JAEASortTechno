#include <DetectorsAdv.h>

#include <Globals.h>

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
}

bool S3Det::fPreferSector = false;
bool S3Det::fAllowMultiHit = true;
bool S3Det::fKeepShared = false;

int S3Det::fRingNumber = 24;
int S3Det::fSectorNumber = 32;
double S3Det::fOffsetPhiCon = 0.5 * kPi;
double S3Det::fOffsetPhiSet = -22.5 * kPi / 180.0;
double S3Det::fOuterDiameter = 70.0;
double S3Det::fInnerDiameter = 22.0;
double S3Det::fTargetDistance = 31.0;
double S3Det::fFrontBackTime = 75.0;
double S3Det::fFrontBackEnergy = 0.9;
double S3Det::fFrontBackOffset = 0.0;

XYZVector S3Hit::Pos(bool smear) const
{
    if(fRingHit == nullptr || fSectorHit == nullptr) {
        return XYZVector(0.0, 0.0, 0.0);
    }

    return S3Det::GetPosition(static_cast<int>(Ring()), static_cast<int>(Sector()), smear);
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
    return std::abs(ring.Time() - sector.Time()) < fFrontBackTime;
}

bool S3Det::EnergyMatches(double ringEnergy, double sectorEnergy) const
{
    return (ringEnergy - fFrontBackOffset) * fFrontBackEnergy < sectorEnergy &&
           (sectorEnergy - fFrontBackOffset) * fFrontBackEnergy < ringEnergy;
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
    if(fPixelsBuilt) {
        return;
    }

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

    if(fAllowMultiHit) {
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
                    const bool adjacentSector = sectorSep == 1 || sectorSep == (fSectorNumber - 1);

                    if(adjacentSector) {
                        // Same ring and neighbour sectors, almost certainly charge sharing.
                        if(fKeepShared) {
                            const DetHit* dominantSector = sectorEnergy[j] >= sectorEnergy[k] ? &fSectorHits[j] : &fSectorHits[k];
                            AddPixel(&fRingHits[i], dominantSector, &fRingHits[i]);
                        }
                    } else {
                        // Two separate hits with a shared ring: the ring is the single side and stays primary.
                        AddPixel(&fRingHits[i], &fSectorHits[j], &fRingHits[i]);
                        AddPixel(&fRingHits[i], &fSectorHits[k], &fRingHits[i]);
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
                    const bool adjacentRing = ringSep == 1;

                    if(adjacentRing) {
                        // Same sector and neighbour rings, almost certainly charge sharing.
                        if(fKeepShared) {
                            const DetHit* dominantRing = ringEnergy[j] >= ringEnergy[k] ? &fRingHits[j] : &fRingHits[k];
                            AddPixel(dominantRing, &fSectorHits[i], &fSectorHits[i]);
                        }
                    } else {
                        // Two separate hits with a shared sector: the sector is the single side and stays primary.
                        AddPixel(&fRingHits[j], &fSectorHits[i], &fSectorHits[i]);
                        AddPixel(&fRingHits[k], &fSectorHits[i], &fSectorHits[i]);
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

XYZVector S3Det::GetPosition(int ring, int sector, bool smear)
{
    const double ringWidth = (fOuterDiameter - fInnerDiameter) * 0.5 / static_cast<double>(fRingNumber);
    const double innerRadius = fInnerDiameter * 0.5;
    const double phiWidth = 2.0 * kPi / static_cast<double>(fSectorNumber);

    double radius = innerRadius + ringWidth * (static_cast<double>(ring) + 0.5);
    double phi = phiWidth * static_cast<double>(sector) + fOffsetPhiCon + fOffsetPhiSet;

    if(smear) {
        const double sep = ringWidth * 0.025;
        const double r1 = radius - 0.5 * ringWidth + sep;
        const double r2 = radius + 0.5 * ringWidth - sep;
        radius = std::sqrt(gThRand().Uniform(r1 * r1, r2 * r2));

        const double phiSep = sep / radius;
        phi = gThRand().Uniform(phi - 0.5 * phiWidth + phiSep, phi + 0.5 * phiWidth - phiSep);
    }

    return XYZVector(std::cos(phi) * radius, std::sin(phi) * radius, fTargetDistance);
}

XYZVector CdTeHit::Pos() const
{
    return XYZVector(0.0, 0.0, 0.0);
}

XYZVector HPGeHit::Pos() const
{
    return XYZVector(0.0, 0.0, 0.0);
}
