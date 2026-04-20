#include <FillHistograms.h>

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
    }

    for(auto& hit : cdte) {
        // H.cdte_ch_adc->Fill(hit.Ch(), hit.Adc());
    }

    for(auto& s3hit : s3.Hits()) {
        // H.s3_ch_adc->Fill(s3hit.Ch(), s3hit.Adc());
    }

    for (size_t i = 0; i < event.Size(); ++i) {
        H.mod_ch_adc_ts->Fill(event.Mod[i], event.Ch[i], event.Adc[i]);

        if (event.Mod[i] == 2) {
            H.mod1_ch_adc->Fill(event.Ch[i], event.Adc[i]);
        }
        if (event.Mod[i] == 1) {
            H.mod2_ch_adc->Fill(event.Ch[i], event.Adc[i]);
            for (size_t j = 0; j < event.Size(); ++j) {
                if (event.Mod[j] == 2) {
                    const double dT = static_cast<double>(event.Ts[i]) - static_cast<double>(event.Ts[j]);
                    H.siall->Fill(event.Ch[j], event.Ch[i]);
                    H.ring_sector_E->Fill(event.Adc[j], event.Adc[i]);
                    H.sector_ring_energy_double->Fill(static_cast<double>(event.Adc[j]), static_cast<double>(event.Adc[i]));
                    if (event.Ch[i] != 11 && event.Ch[i] != 16 && event.Ch[i] != 17 && event.Ch[i] != 18) {
                        H.ring_sector_E_reduced->Fill(event.Adc[j], event.Adc[i]);
                        H.sidt->Fill(dT);
                    }
                    if (dT > -100.0 && dT < 100.0) {
                        H.pmpt_ring_sector_E->Fill(event.Adc[j], event.Adc[i]);
                        if (event.Ch[i] != 11 && event.Ch[i] != 16 && event.Ch[i] != 17 && event.Ch[i] != 18) {
                            H.pmpt_ring_sector_E_reduced->Fill(event.Adc[j], event.Adc[i]);
                        }

                        if (event.Adc[i] != 0) {
                            H.hSectE_divRingE->Fill(static_cast<double>(event.Adc[j]) / static_cast<double>(event.Adc[i]));
                        }

                        if (event.Ch[i] < 4) {
                            H.ESumPart[event.Ch[i]]->Fill(event.Adc[i] + event.Adc[j], event.Adc[i]);
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
                    H.hRingRing->Fill(event.Ch[i], event.Ch[j]);
                }
            }
        }
        if (event.Mod[i] == 2) {
            for (size_t j = 0; j < event.Size(); ++j) {
                if (event.Mod[j] == 2 && i != j) {
                    H.hSectSect->Fill(event.Ch[i], event.Ch[j]);
                }
            }
        }
        if (event.Mod[i] == 3) {
            H.mod3_ch_adc->Fill(event.Ch[i], event.Adc[i]);
        }
        if (event.Mod[i] == 4) {
            H.mod4_ch_adc->Fill(event.Ch[i], event.Adc[i]);
        }
    }
}
