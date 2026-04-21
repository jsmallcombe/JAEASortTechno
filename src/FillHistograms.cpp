#include <FillHistograms.h>

#include <cmath>

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

void FillHistograms(HistogramRefs& H, const BuiltEventView& event)
{
    DetHitScratch& detHits = BuildDetHitCategories(event);
    auto& hpge = detHits.hpge;
    auto& cdte = detHits.cdte;
    auto& s3 = detHits.s3;

    for(auto& hit : hpge) {
        // H.hpge_ch_adc->Fill(hit.Ch(), hit.Adc());
        H.hpge_chan->Fill(hit.Index(), hit.Energy());
    }

    for(auto& hit : cdte) {
        // H.cdte_ch_adc->Fill(hit.Ch(), hit.Adc());
        H.cdte_chan->Fill(hit.Index(), hit.Energy());
    }

    H.s3_raw_ring_mult->Fill(s3.GetRingMultiplicity());
    H.s3_raw_sector_mult->Fill(s3.GetSectorMultiplicity());
    H.s3_pixel_mult->Fill(s3.GetPixelMultiplicity());

    for(auto& sector : s3.SectorHits()) {
        H.s3_raw_sector_energy->Fill(sector.Index(), sector.Energy());
    }
    
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
        }
    }

    for(auto& s3hit : s3.Hits()) {
        H.s3_pixel_ring_sector->Fill(s3hit.Sector(), s3hit.Ring());
        H.s3_pixel_energy->Fill(s3hit.Energy());
        H.s3_pixel_ring_energy->Fill(s3hit.Ring(), s3hit.Energy());
        H.s3_pixel_sector_energy->Fill(s3hit.Sector(), s3hit.Energy());
        const XYZVector pos = s3hit.Pos(true);
        H.s3_pixel_theta_energy->Fill(pos.Theta(), s3hit.Energy());
        H.s3_pixel_position_xy->Fill(pos.X(), pos.Y());
        H.s3_pixel_position_xyz->Fill(pos.X(), pos.Y(), pos.Z());

        const DetHit* ring = s3hit.RingHit();
        const DetHit* sector = s3hit.SectorHit();
        if(ring != nullptr && sector != nullptr) {
            H.s3_pixel_ring_sector_energy->Fill(ring->Energy(), sector->Energy());
            H.s3_pixel_ring_sector_dt->Fill(ring->Time() - sector->Time());
        }

        for(auto& hit : hpge) {
            const double dT = hit.Time() - s3hit.Time();
            H.hpge_S3time->Fill(hit.Index(), dT);
            if(dT > -100.0 && dT < 100.0) {
                H.hpge_S3time_gate->Fill(hit.Index(), dT);
                H.hpge_S3->Fill(hit.Index(), hit.Energy());
            }
        }

        for(auto& hit : cdte) {
            const double dT = hit.Time() - s3hit.Time();
            H.cdte_S3time->Fill(hit.Index(), dT);
            if(dT > -100.0 && dT < 100.0) {
                H.cdte_S3time_gate->Fill(hit.Index(), dT);
                H.cdte_S3->Fill(hit.Index(), hit.Energy());
            }
        }

    }

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
