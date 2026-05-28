#include <FillHistogramsBasic.h>

#include <HistogramRuntime.h>

void FillHistogramsBasic(HistogramRefsBasic& H, const BuiltEventView& event)
{
    HistogramScopedTimer totalTimer(gHistogramRuntimeTimers.fillBasicNs);

    for (size_t i = 0; i < event.Size(); ++i) {
        const UShort_t mod = event.Mod[i];
        const UShort_t ch = event.Ch[i];

        if (mod < 4) {
            H.ModulesRaw[mod]->Fill(ch, event.Adc[i]);
        }

        if (mod == 1) {
            for (size_t j = 0; j < event.Size(); ++j) {
                if (event.Mod[j] == 0) {
                    const double dT = static_cast<double>(event.Ts[i]) - static_cast<double>(event.Ts[j]);
                    H.siall->Fill(event.Ch[j], ch);
                    H.ring_sector_E->Fill(event.Adc[j], event.Adc[i]);
                    H.sector_ring_energy_double->Fill(static_cast<double>(event.Adc[j]),
                                                      static_cast<double>(event.Adc[i]));
                    H.ring_sector_E_reduced->Fill(event.Adc[j], event.Adc[i]);
                    H.sidt->Fill(dT);

                    if (dT > -100.0 && dT < 100.0) {
                        H.pmpt_ring_sector_E->Fill(event.Adc[j], event.Adc[i]);

                        //H.pmpt_ring_sector_E_reduced->Fill(event.Adc[j], event.Adc[i]);

                        if (event.Adc[i] != 0) {
                            H.hSectE_divRingE->Fill(static_cast<double>(event.Adc[j]) /
                                                    static_cast<double>(event.Adc[i]));
                        }

                        if (ch < 4) {
                            H.ESumPart[ch]->Fill(event.Adc[i] + event.Adc[j], event.Adc[i]);
                        }

                        if (event.Adc[i] > 120 && event.Adc[j] > 120) {
                            for (size_t k = 0; k < event.Size(); ++k) {
                                if (event.Mod[k] == 2) {
                                    const double dT_sect_cdte = static_cast<double>(event.Ts[j]) -
                                                                static_cast<double>(event.Ts[k]);
                                    H.hSect_CdTe_dT->Fill(dT_sect_cdte);
                                    H.hSect_CdTe_dT_ADC->Fill(dT_sect_cdte, event.Adc[k]);
                                }
                                if (event.Mod[k] == 3) {
                                    const double dT_sect_hpge = static_cast<double>(event.Ts[j]) -
                                                                static_cast<double>(event.Ts[k]);
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
        if (mod == 0) {
            for (size_t j = 0; j < event.Size(); ++j) {
                if (event.Mod[j] == 0 && i != j) {
                    H.hSectSect->Fill(ch, event.Ch[j]);
                }
            }
        }
    }
}
