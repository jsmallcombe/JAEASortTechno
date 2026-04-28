#include <FillHistograms.h>

#include <IO.h>

#include <Math/VectorUtil.h>
#include <TMath.h>

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
            gateRefs.betatheta = gIO->GetGateConst(1);
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
    // Retrieve gate values from IO, initialized on the first thread call
    const HistogramGateRefs& gates = HistogramGateRefsBuffer();

    // Build detector objects from basic events data`
    DetHitScratch& detHits = BuildDetHitCategories(event);
    auto& hpge = detHits.hpge;
    auto& cdte = detHits.cdte;
    auto& s3 = detHits.s3;
 
    for(size_t i = 0; i < cdte.size(); ++i) {
        auto& hit = cdte[i];

        // H.cdte_ch_adc->Fill(hit.Ch(), hit.Adc());
        H.cdte_chan->Fill(hit.Index(), hit.Energy());
        H.cdte_energy->Fill(hit.Energy());
        const XYZVector pos = hit.Pos(true);
        H.gamma_positions->Fill(pos.Z(), pos.X(), pos.Y());

        for(size_t j = i + 1; j < cdte.size(); ++j) {
            auto& hitb = cdte[j];

            const double dT = hit.Time() - hitb.Time();
            H.cdte_cdte_dt->Fill(dT);
            if(TMath::Abs(dT) < 100.0) {
                H.cdte_cdte_dt_gate->Fill(dT);
                H.cdte_cdte->Fill(hit.Energy(), hitb.Energy());
                H.cdte_cdte->Fill(hitb.Energy(), hit.Energy());
            }//cdte.cdte.dt
        }//cdte.cdte

        for(auto&& hpgeHit : hpge) {
            const double dT = hit.Time() - hpgeHit.Time();
            H.cdte_hpge_dt->Fill(dT);
            if(TMath::Abs(dT) < 100.0) {
                H.cdte_hpge_dt_gate->Fill(dT);
                H.cdte_hpge->Fill(hit.Energy(), hpgeHit.Energy());
            }//cdte.hpge.dt
        }//cdte.hpge
    }//cdte

    for(size_t i = 0; i < hpge.size(); ++i) {
        auto& hit = hpge[i];

        H.hpge_chan->Fill(hit.Index(), hit.Energy());
        H.hpge_energy->Fill(hit.Energy());
        const XYZVector pos = hit.Pos(true);
        H.gamma_positions->Fill(pos.Z(), pos.X(), pos.Y());

        for(size_t j = i + 1; j < hpge.size(); ++j) {
            auto& hitb = hpge[j];

            const double dT = hit.Time() - hitb.Time();
            H.hpge_hpge_dt->Fill(dT);
            if(TMath::Abs(dT) < 100.0) {
                H.hpge_hpge_dt_gate->Fill(dT);
                H.hpge_hpge->Fill(hit.Energy(), hitb.Energy());
                H.hpge_hpge->Fill(hitb.Energy(), hit.Energy());
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

    for(auto& s3hit : s3.Hits()) {
        H.s3_pixel_ring_sector->Fill(s3hit.Sector(), s3hit.Ring());
        H.s3_pixel_energy->Fill(s3hit.Energy());
        H.s3_pixel_ring_energy->Fill(s3hit.Ring(), s3hit.Energy());
        H.s3_pixel_sector_energy->Fill(s3hit.Sector(), s3hit.Energy());
        const XYZVector pos = s3hit.Pos(true);
        H.s3_pixel_theta_energy->Fill(pos.Theta(), s3hit.Energy());
        H.s3_pixel_position_xy->Fill(pos.X(), pos.Y());
        H.s3_pixel_position_xyz->Fill(pos.Z(), pos.X(), pos.Y());
        XYZVector s3KinPos = pos;

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

        for(auto& hit : hpge) {
            const double dT = hit.Time() - s3hit.Time();
            H.hpge_S3time->Fill(hit.Index(), dT);
            if(dT > gates.hpgeS3down && dT < gates.hpgeS3up) {
                H.hpge_S3time_gate->Fill(hit.Index(), dT);
                H.hpge_S3->Fill(hit.Index(), hit.Energy());
                H.hpge_energy_S3->Fill(hit.Energy());
                const UShort_t index = hit.Index();
                if(index < 6) {
                    const XYZVector gammaPos = hit.Pos(true);
                    const double openingAngle = ROOT::Math::VectorUtil::Angle(s3KinPos, gammaPos) * TMath::RadToDeg();
                    H.hpge_kinematics->Fill(openingAngle, hit.Energy());
                    H.HPGeKinematics[index]->Fill(openingAngle, hit.Energy());
                }//s3.hpge.dt.hpgepixel
            }//s3.hpge.dt
        }//s3.hpge

        for(auto& cdteHit : cdte) {
            const double dT = cdteHit.Time() - s3hit.Time();
            H.cdte_S3time->Fill(cdteHit.Index(), dT);
            if(dT > gates.cdteS3down && dT < gates.cdteS3up) {
                H.cdte_S3time_gate->Fill(cdteHit.Index(), dT);
                H.cdte_S3->Fill(cdteHit.Index(), cdteHit.Energy());
                H.cdte_energy_S3->Fill(cdteHit.Energy());
                const UShort_t index = cdteHit.Index();
                if(index < 16) {
                    const XYZVector gammaPos = cdteHit.Pos(true);
                    const double openingAngle = ROOT::Math::VectorUtil::Angle(s3KinPos, gammaPos) * TMath::RadToDeg();
                    H.cdte_kinematics->Fill(openingAngle, cdteHit.Energy());
                    H.CdTeKinematics[index]->Fill(openingAngle, cdteHit.Energy());
                }//s3.cdte.dt.cdpixel

                for(auto&& hpgeHit : hpge) {
                    const double hpgeS3dT = hpgeHit.Time() - s3hit.Time();
                    if(hpgeS3dT > gates.hpgeS3down && hpgeS3dT < gates.hpgeS3up) {
                        H.cdte_hpge_S3->Fill(cdteHit.Energy(), hpgeHit.Energy());
                    }//s3.cdte.dt.hpge.dt
                }//s3.cdte.dt.hpge
            }//s3.cdte.dt
        }//s3.cdte
    }//s3hit

    for (size_t i = 0; i < event.Size(); ++i) {

        const UShort_t mod = event.Mod[i];
        const UShort_t ch = event.Ch[i];

        if(mod <4 ) {
            H.ModulesRaw[mod]->Fill(ch, event.Adc[i]);
        }

        if (mod == 1) {
            for (size_t j = 0; j < event.Size(); ++j) {
                if (event.Mod[j] == 2) {
                    const double dT = static_cast<double>(event.Ts[i]) - static_cast<double>(event.Ts[j]);
                    H.siall->Fill(event.Ch[j], ch);
                    H.ring_sector_E->Fill(event.Adc[j], event.Adc[i]);
                    H.sector_ring_energy_double->Fill(static_cast<double>(event.Adc[j]), static_cast<double>(event.Adc[i]));
                    if (ch != 11 && ch != 16 && ch != 17 && ch != 18) {
                        H.ring_sector_E_reduced->Fill(event.Adc[j], event.Adc[i]);
                        H.sidt->Fill(dT);
                    }
                    if (dT > -100.0 && dT < 100.0) {
                        H.pmpt_ring_sector_E->Fill(event.Adc[j], event.Adc[i]);
                        if (ch != 11 && ch != 16 && ch != 17 && ch != 18) {
                            H.pmpt_ring_sector_E_reduced->Fill(event.Adc[j], event.Adc[i]);
                        }

                        if (event.Adc[i] != 0) {
                            H.hSectE_divRingE->Fill(static_cast<double>(event.Adc[j]) / static_cast<double>(event.Adc[i]));
                        }

                        if (ch < 4) {
                            H.ESumPart[ch]->Fill(event.Adc[i] + event.Adc[j], event.Adc[i]);
                        }

                        if (event.Adc[i] > 120 && event.Adc[j] > 120) {
                            for (size_t k = 0; k < event.Size(); ++k) {
                                if (event.Mod[k] == 3 && event.Ch[k] > 7) {
                                    const double dT_sect_cdte = static_cast<double>(event.Ts[j]) - static_cast<double>(event.Ts[k]);
                                    H.hSect_CdTe_dT->Fill(dT_sect_cdte);
                                    H.hSect_CdTe_dT_ADC->Fill(dT_sect_cdte, event.Adc[k]);
                                }
                                if (event.Mod[k] == 4) {
                                    const double dT_sect_hpge = static_cast<double>(event.Ts[j]) - static_cast<double>(event.Ts[k]);
                                    H.hSect_HPGe_dT->Fill(dT_sect_hpge);
                                    H.hSect_HPGe_dT_ADC->Fill(dT_sect_hpge, event.Adc[k]);
                                }
                            }
                        }
                    }
                }
                if (event.Mod[j] == 1 && j != i) {
                    H.hRingRing->Fill(ch, event.Ch[j]);
                }
            }
        }
        if (mod == 2) {
            for (size_t j = 0; j < event.Size(); ++j) {
                if (event.Mod[j] == 2 && i != j) {
                    H.hSectSect->Fill(ch, event.Ch[j]);
                }
            }
        }
    }
}
