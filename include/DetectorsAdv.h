#ifndef JAEASortDetectorsAdv
#define JAEASortDetectorsAdv

#include <BuiltEvent.h>
#include <Detectors.h>
#include <Math/Vector3D.h>

#include <cstddef>
#include <vector>

using XYZVector = ROOT::Math::XYZVector;

double DopplerCorrectEnergy(double energyKeV, double angleRad, double beta);

class DetPos {
protected:
    mutable bool fPosSet{false};
    mutable XYZVector fPos{0.0, 0.0, 0.0};
    mutable XYZVector fBlurPos{0.0, 0.0, 0.0};

    virtual void BuildPos() const = 0;

public:
    DetPos() = default;
    virtual ~DetPos() = default;

    DetPos(const DetPos&) { ResetPos(); }
    DetPos& operator=(const DetPos&)
    {
        ResetPos();
        return *this;
    }
    DetPos(DetPos&&) noexcept { ResetPos(); }
    DetPos& operator=(DetPos&&) noexcept
    {
        ResetPos();
        return *this;
    }

    void ResetPos() const;
    XYZVector Pos(bool smear = false) const;
    XYZVector GetPos(bool smear = false) const { return Pos(smear); };
};

class CdTeHit : public DetHit, public DetPos {
public:
    CdTeHit() = default;
    CdTeHit(Double_t tTS, UShort_t tC, UShort_t tMod, UShort_t tChan) : DetHit(tTS, tC, tMod, tChan), DetPos() {}
    virtual ~CdTeHit() = default;

    UShort_t DetectorNumber() const;
    UShort_t DetectorPixel() const;
    bool IsNeighbour(const CdTeHit& other) const;
    Double_t DopplerCorrectedEnergy(double angleRad, double beta) const;
    static XYZVector PosStatic(bool smear = false, u_short i = 0, XYZVector* pos = nullptr);
    static void XOffset(double offset);
    static void YOffset(double offset);
    static void ZOffset(double offset);

protected:
    void BuildPos() const override;
};

class HPGeHit : public DetHit, public DetPos {
public:
    HPGeHit() = default;
    HPGeHit(Double_t tTS, UShort_t tC, UShort_t tMod, UShort_t tChan) : DetHit(tTS, tC, tMod, tChan), DetPos() {}
    virtual ~HPGeHit() = default;

    Double_t DopplerCorrectedEnergy(double angleRad, double beta) const;
    static XYZVector PosStatic(bool smear = false, u_short i = 0, XYZVector* pos = nullptr);

protected:
    void BuildPos() const override;
};

class S3Hit : public DetPos {
private:
    const DetHit* fRingHit{nullptr};
    const DetHit* fSectorHit{nullptr};
    const DetHit* fPrimary{nullptr};

public:
    S3Hit() = default;
    S3Hit(const DetHit* ring, const DetHit* sector, const DetHit* primary = nullptr)
        : fRingHit(ring), fSectorHit(sector), fPrimary(primary != nullptr ? primary : ring)
    {
    }
    virtual ~S3Hit() = default;

    const DetHit* RingHit() const { return fRingHit; }
    const DetHit* SectorHit() const { return fSectorHit; }
    const DetHit* PrimaryHit() const { return fPrimary; }

    UShort_t Ring() const { return fRingHit != nullptr ? fRingHit->Index() : 0; }
    UShort_t Sector() const { return fSectorHit != nullptr ? fSectorHit->Index() : 0; }

    Double_t Energy() const { return fPrimary != nullptr ? fPrimary->Energy() : 0.0; }
    Double_t Time() const { return fPrimary != nullptr ? fPrimary->Time() : 0.0; }

protected:
    void BuildPos() const override;
};

class S3Det {
public:
    S3Det() = default;
    virtual ~S3Det() = default;

    void Clear();
    void AddHit(const DetHit& hit);
    void AddHits(const std::vector<DetHit>& hits);
    void AddHit(Double_t tTS, UShort_t tC, UShort_t tMod, UShort_t tChan);
    void AddHit(const BuiltEventView& event, std::size_t i);
    void AddRingHit(const DetHit& hit);
    void AddSectorHit(const DetHit& hit);
    void AddRingHit(Double_t tTS, UShort_t tC, UShort_t tMod, UShort_t tChan);
    void AddRingHit(const BuiltEventView& event, std::size_t i);
    void AddSectorHit(Double_t tTS, UShort_t tC, UShort_t tMod, UShort_t tChan);
    void AddSectorHit(const BuiltEventView& event, std::size_t i);
    void ResetRingsSectors();

    std::size_t GetRingMultiplicity() const { return fRingHits.size(); }
    std::size_t GetSectorMultiplicity() const { return fSectorHits.size(); }
    std::size_t GetPixelMultiplicity();

    const std::vector<DetHit>& RingHits() const { return fRingHits; }
    const std::vector<DetHit>& SectorHits() const { return fSectorHits; }
    const std::vector<S3Hit>& Hits();

    const S3Hit* GetS3Hit(std::size_t i);
    const DetHit* GetRingHit(std::size_t i) const;
    const DetHit* GetSectorHit(std::size_t i) const;

    static bool fPreferSector;
    static bool fAllowMultiHit;
    static bool fKeepShared;
    static bool fFlipPhi;

    static int fRingNumber;
    static int fSectorNumber;
    static double fOffsetPhiCon;
    static double fOffsetPhiSet;
    static double fOuterDiameter;
    static double fInnerDiameter;
    static double fTargetDistance;
    static double fXOffset;
    static double fYOffset;

    static double fFrontBackTime;
    static double fFrontBackEnergy;
    static double fFrontBackOffset;

    static XYZVector GetPosition(int ring, int sector, bool smear = false, XYZVector* pos = nullptr);

private:
    void BuildHits();
    bool TimeMatches(const DetHit& ring, const DetHit& sector) const;
    bool EnergyMatches(double ringEnergy, double sectorEnergy) const;
    void AddPixel(const DetHit* ring, const DetHit* sector, const DetHit* primary = nullptr);

    std::vector<DetHit> fRingHits;
    std::vector<DetHit> fSectorHits;
    std::vector<S3Hit> fHits;
    bool fPixelsBuilt{false};

};

#endif
