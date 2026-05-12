#include <FillHistograms.h>

#include <HistogramRuntime.h>
#include <IO.h>

#include <Math/VectorUtil.h>
#include <TMath.h>

#include <optional>

DetHitScratch& DetHitScratchBuffer()
{
    thread_local DetHitScratch scratch;
    scratch.Clear();
    return scratch;
}

DetHitScratch& BuildDetHitCategories(const BuiltEventView& event)
{
    DetHitScratch& scratch = DetHitScratchBuffer();

    for (size_t i = 0; i < event.Size(); ++i) {
        const UShort_t mod = event.Mod[i];
        const UShort_t ch = event.Ch[i];

        switch (DetHit::GetDetType(mod, ch)) {
            case DetHit::HPGe:
                scratch.hpge.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::CdTe:
                scratch.cdte.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::S3Ring:
                scratch.s3.AddRingHit(event.Ts[i], event.Adc[i], mod, ch);
                break;
            case DetHit::S3Sector:
                scratch.s3.AddSectorHit(event.Ts[i], event.Adc[i], mod, ch);
                break;
            default:
                scratch.hits.emplace_back(event.Ts[i], event.Adc[i], mod, ch);
                break;
        }
    }
    
    return scratch;
}

const HistogramGateRefs& HistogramGateRefsBuffer()
{
    thread_local HistogramGateRefs gateRefs;
    thread_local bool initialized = false;

    if (!initialized) {
        if (gIO != nullptr) {
            gateRefs.invkin = gIO->GetGateConst(0);
            gateRefs.beta = gIO->GetGateConst(1);
            gateRefs.betabeam = gIO->GetGateConst(2);
            gateRefs.cdteS3up = gIO->GetInput("CdTeS3Up", 100);
            gateRefs.cdteS3down = gIO->GetInput("CdTeS3Down", -100);
            gateRefs.hpgeS3up = gIO->GetInput("HpGeS3Up", 100);
            gateRefs.hpgeS3down = gIO->GetInput("HpGeS3Down", -100);
        }
        initialized = true;
    }
    return gateRefs;
}

void FillHistograms(HistogramRefs& H, const BuiltEventView& event)
{
    const bool enableTimers = gHistogramRuntimeOptions.enableHistogramTimers;
    std::optional<HistogramChunkTimer> chunkTimer;
    if (enableTimers) chunkTimer.emplace(gHistogramRuntimeTimers.fillHistChunk1Ns);

    // Retrieve gate values from IO, initialized on the first thread call
    const HistogramGateRefs& gates = HistogramGateRefsBuffer();

    // Move these Next(...) lines to retime the section boundaries.
    if (enableTimers) chunkTimer->Next(gHistogramRuntimeTimers.fillHistChunk2Ns);

    // Build detector objects from basic events data`
    DetHitScratch& detHits = BuildDetHitCategories(event);
    auto& hpge = detHits.hpge;
    auto& cdte = detHits.cdte;
    auto& s3 = detHits.s3;
 
    if (enableTimers) chunkTimer->Next(gHistogramRuntimeTimers.fillHistChunk3Ns);

    for(size_t i = 0; i < cdte.size(); ++i) {
        auto& hit = cdte[i];
        const double hitEnergy = hit.Energy();
        const double hitTime = hit.Time();

        // H.cdte_ch_adc->Fill(hit.Ch(), hit.Adc());
        H.cdte_chan->Fill(hit.Index(), hitEnergy);
        H.cdte_energy->Fill(hitEnergy);

        for(size_t j = i + 1; j < cdte.size(); ++j) {
            auto& hitb = cdte[j];
            const double hitbTime = hitb.Time();

            const double dT = hitTime - hitbTime;
            H.cdte_cdte_dt->Fill(dT);
            if(std::abs(dT) < 100.0) {
                const double hitbEnergy = hitb.Energy();
                H.cdte_cdte_dt_gate->Fill(dT);
                H.cdte_cdte->Fill(hitEnergy, hitbEnergy);
                H.cdte_cdte->Fill(hitbEnergy, hitEnergy);
            }//cdte.cdte.dt
        }//cdte.cdte

        for(auto&& hpgeHit : hpge) {
            const double dT = hitTime - hpgeHit.Time();
            H.cdte_hpge_dt->Fill(dT);
            if(std::abs(dT) < 100.0) {
                H.cdte_hpge_dt_gate->Fill(dT);
                H.cdte_hpge->Fill(hitEnergy, hpgeHit.Energy());
            }//cdte.hpge.dt
        }//cdte.hpge
    }//cdte


    for(size_t i = 0; i < hpge.size(); ++i) {
        auto& hit = hpge[i];
        const double hitEnergy = hit.Energy();
        const double hitTime = hit.Time();

        H.hpge_chan->Fill(hit.Index(), hitEnergy);
        H.hpge_energy->Fill(hitEnergy);

        for(size_t j = i + 1; j < hpge.size(); ++j) {
            auto& hitb = hpge[j];

            const double dT = hitTime - hitb.Time();
            H.hpge_hpge_dt->Fill(dT);
            if(std::abs(dT) < 100.0) {
                const double hitbEnergy = hitb.Energy();
                H.hpge_hpge_dt_gate->Fill(dT);
                H.hpge_hpge->Fill(hitEnergy, hitbEnergy);
                H.hpge_hpge->Fill(hitbEnergy, hitEnergy);
            }//hpge.hpge.dt
        }//hpge.hpge
    }//hpge


    H.s3_raw_ring_mult->Fill(s3.GetRingMultiplicity());
    H.s3_raw_sector_mult->Fill(s3.GetSectorMultiplicity());
    H.s3_pixel_mult->Fill(s3.GetPixelMultiplicity());

    for(auto& sector : s3.SectorHits()) {
        H.s3_raw_sector_energy->Fill(sector.Index(), sector.Energy());
    }//s3sector
    
    for(auto& ring : s3.RingHits()) {
        H.s3_raw_ring_energy->Fill(ring.Index(), ring.Energy());

        for(auto& sector : s3.SectorHits()) {
            const double dT = ring.Time() - sector.Time();
            H.s3_raw_ring_sector->Fill(sector.Index(), ring.Index());
            H.s3_raw_ring_sector_energy->Fill(ring.Energy(), sector.Energy());
            H.s3_raw_sector_vs_sector_ring_energy->Fill(sector.Index(), sector.Energy(), ring.Energy());
            H.s3_raw_ring_vs_sector_ring_energy->Fill(ring.Index(), sector.Energy(), ring.Energy());
            H.s3_raw_ring_sector_dt->Fill(dT);
            H.s3_raw_ring_dt->Fill(ring.Index(), dT);
            H.s3_raw_sector_dt->Fill(sector.Index(), dT);
        }//s3ring.s3sector
    }//s3ring


    if (enableTimers) chunkTimer->Next(gHistogramRuntimeTimers.fillHistChunk4Ns);

    std::vector<const HPGeHit*> gatedHpgeHits;
    gatedHpgeHits.reserve(hpge.size());
    for(auto& s3hit : s3.Hits()) {
        const UShort_t s3Sector = s3hit.Sector();
        const UShort_t s3Ring = s3hit.Ring();
        const double s3Energy = s3hit.Energy();
        const double s3Time = s3hit.Time();

        H.s3_pixel_ring_sector->Fill(s3Sector, s3Ring);
        H.s3_pixel_energy->Fill(s3Energy);
        H.s3_pixel_ring_energy->Fill(s3Ring, s3Energy);
        H.s3_pixel_sector_energy->Fill(s3Sector, s3Energy);
        const XYZVector pos = s3hit.Pos(true);
        H.s3_pixel_theta_energy->Fill(pos.Theta(), s3Energy);
        H.s3_pixel_position_xy->Fill(pos.X(), pos.Y());
        H.s3_pixel_position_xyz->Fill(pos.Z(), pos.X(), pos.Y());
        XYZVector s3KinPos = pos;
        const double beta = gates.beta != nullptr ? gates.beta->Eval(pos.Theta()) : 0.0;
        const double betaBeam = gates.betabeam != nullptr ? gates.betabeam->Eval(pos.Theta()) : 0.0;

        const DetHit* ring = s3hit.RingHit();
        const DetHit* sector = s3hit.SectorHit();
        if(ring != nullptr && sector != nullptr) {
            H.s3_pixel_ring_sector_energy->Fill(ring->Energy(), sector->Energy());
            H.s3_pixel_ring_sector_dt->Fill(ring->Time() - sector->Time());
        }

        if(gates.invkin){
            const double theta = gates.invkin->Eval(pos.Theta());
            const double phi = pos.Phi() + TMath::Pi();
            s3KinPos = ROOT::Math::Polar3DVector(1.0, theta, phi);
        }

        gatedHpgeHits.clear();
        for(auto& hit : hpge) {
            const double dT = hit.Time() - s3Time;
            H.hpge_S3time->Fill(hit.Index(), dT);
            if(dT > gates.hpgeS3down && dT < gates.hpgeS3up) {
                gatedHpgeHits.push_back(&hit);
                
                // Moved here from hpge raw as position maths is a bit heavy, so limit the calls
                const XYZVector gammaPos = hit.Pos(true);
                H.gamma_positions->Fill(gammaPos.Z(), gammaPos.X(), gammaPos.Y());

                H.hpge_S3time_gate->Fill(hit.Index(), dT);
                H.hpge_S3->Fill(hit.Index(), hit.Energy());
                H.hpge_energy_S3->Fill(hit.Energy());
                const double openingAngle = ROOT::Math::VectorUtil::Angle(s3KinPos, gammaPos);
                const double dopplerEnergy = hit.DopplerCorrectedEnergy(openingAngle, beta);
                const double openingAngleBeam = ROOT::Math::VectorUtil::Angle(pos, gammaPos);
                const double dopplerEnergyBeam = hit.DopplerCorrectedEnergy(openingAngleBeam, betaBeam);
                H.hpge_doppler->Fill(dopplerEnergy);
                H.hpge_ring_doppler->Fill(s3Ring, dopplerEnergy);
                H.hpge_doppler_beam->Fill(dopplerEnergyBeam);
                H.hpge_ring_doppler_beam->Fill(s3Ring, dopplerEnergyBeam);
                if(s3Ring < kS3RingKinematicsCount) {
                    H.HPGeKinematics[s3Ring]->Fill(openingAngle * TMath::RadToDeg(), hit.Energy());
                    H.HPGeKinematicsBeam[s3Ring]->Fill(openingAngleBeam * TMath::RadToDeg(), hit.Energy());
                }//s3.hpge.dt.hpgepixel
            }//s3.hpge.dt
        }//s3.hpge


        for(auto& cdteHit : cdte) {
            const double dT = cdteHit.Time() - s3Time;
            H.cdte_S3time->Fill(cdteHit.Index(), dT);
            if(dT > gates.cdteS3down && dT < gates.cdteS3up) {
                const double cdteEnergy = cdteHit.Energy();

                const XYZVector gammaPos = cdteHit.Pos(true);
                H.gamma_positions->Fill(gammaPos.Z(), gammaPos.X(), gammaPos.Y());

                H.cdte_S3time_gate->Fill(cdteHit.Index(), dT);
                H.cdte_S3->Fill(cdteHit.Index(), cdteEnergy);
                H.cdte_energy_S3->Fill(cdteEnergy);
                const double openingAngle = ROOT::Math::VectorUtil::Angle(s3KinPos, gammaPos);
                const double dopplerEnergy = cdteHit.DopplerCorrectedEnergy(openingAngle, beta);
                const double openingAngleBeam = ROOT::Math::VectorUtil::Angle(pos, gammaPos);
                const double dopplerEnergyBeam = cdteHit.DopplerCorrectedEnergy(openingAngleBeam, betaBeam);
                H.cdte_doppler->Fill(dopplerEnergy);
                H.cdte_ring_doppler->Fill(s3Ring, dopplerEnergy);
                H.cdte_doppler_beam->Fill(dopplerEnergyBeam);
                H.cdte_ring_doppler_beam->Fill(s3Ring, dopplerEnergyBeam);
                if(s3Ring < kS3RingKinematicsCount) {
                    H.CdTeKinematics[s3Ring]->Fill(openingAngle * TMath::RadToDeg(), cdteEnergy);
                    H.CdTeKinematicsBeam[s3Ring]->Fill(openingAngleBeam * TMath::RadToDeg(), cdteEnergy);
                }//s3.cdte.dt.cdpixel

                for(const HPGeHit* hpgeHit : gatedHpgeHits) {
                    H.cdte_hpge_S3->Fill(cdteEnergy, hpgeHit->Energy());
                }//s3.cdte.dt.hpge
            }//s3.cdte.dt
        }//s3.cdte

    }//s3hit
}
