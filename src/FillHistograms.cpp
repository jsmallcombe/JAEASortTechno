#include <FillHistograms.h>

#include <ROOT/TTreeProcessorMT.hxx>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>

#include <thread>

void FillHistograms(ThreadedHistogramSet& histograms, const BuiltEventView& event)
{
    auto siall = histograms.siall.Get();
    auto sidt = histograms.sidt.Get();
    auto ring_sector_E = histograms.ring_sector_E.Get();
    auto ring_sector_E_reduced = histograms.ring_sector_E_reduced.Get();
    auto pmpt_ring_sector_E = histograms.pmpt_ring_sector_E.Get();
    auto pmpt_ring_sector_E_reduced = histograms.pmpt_ring_sector_E_reduced.Get();
    auto hSect_CdTe_dT = histograms.hSect_CdTe_dT.Get();
    auto hSect_HPGe_dT = histograms.hSect_HPGe_dT.Get();
    auto hSect_CdTe_dT_ADC = histograms.hSect_CdTe_dT_ADC.Get();
    auto hSect_HPGe_dT_ADC = histograms.hSect_HPGe_dT_ADC.Get();
    auto hRingRing = histograms.hRingRing.Get();
    auto hSectSect = histograms.hSectSect.Get();
    auto hSectE_divRingE = histograms.hSectE_divRingE.Get();
    auto mod1_ch_adc = histograms.mod1_ch_adc.Get();
    auto mod2_ch_adc = histograms.mod2_ch_adc.Get();
    auto mod3_ch_adc = histograms.mod3_ch_adc.Get();
    auto mod4_ch_adc = histograms.mod4_ch_adc.Get();
    auto mod_ch_adc_ts = histograms.mod_ch_adc_ts.Get();
    auto sector_ring_energy_double = histograms.sector_ring_energy_double.Get();
    std::shared_ptr<TH2F> eSumPart[4];
    for (int i = 0; i < 4; ++i) {
        eSumPart[i] = histograms.ESumPart[i]->Get();
    }

    for (size_t i = 0; i < event.Size(); ++i) {
        mod_ch_adc_ts->Fill(event.Mod[i], event.Ch[i], event.Adc[i]);

        if (event.Mod[i] == 2) {
            mod1_ch_adc->Fill(event.Ch[i], event.Adc[i]);
        }
        if (event.Mod[i] == 1) {
            mod2_ch_adc->Fill(event.Ch[i], event.Adc[i]);
            for (size_t j = 0; j < event.Size(); ++j) {
                if (event.Mod[j] == 2) {
                    const double dT = static_cast<double>(event.Ts[i]) - static_cast<double>(event.Ts[j]);
                    siall->Fill(event.Ch[j], event.Ch[i]);
                    ring_sector_E->Fill(event.Adc[j], event.Adc[i]);
                    sector_ring_energy_double->Fill(static_cast<double>(event.Adc[j]), static_cast<double>(event.Adc[i]));
                    if (event.Ch[i] != 11 && event.Ch[i] != 16 && event.Ch[i] != 17 && event.Ch[i] != 18) {
                        ring_sector_E_reduced->Fill(event.Adc[j], event.Adc[i]);
                        sidt->Fill(dT);
                    }
                    if (dT > -100.0 && dT < 100.0) {
                        pmpt_ring_sector_E->Fill(event.Adc[j], event.Adc[i]);
                        if (event.Ch[i] != 11 && event.Ch[i] != 16 && event.Ch[i] != 17 && event.Ch[i] != 18) {
                            pmpt_ring_sector_E_reduced->Fill(event.Adc[j], event.Adc[i]);
                        }

                        if (event.Adc[i] != 0) {
                            hSectE_divRingE->Fill(static_cast<double>(event.Adc[j]) / static_cast<double>(event.Adc[i]));
                        }

                        if (event.Ch[i] < 4) {
                            eSumPart[event.Ch[i]]->Fill(event.Adc[i] + event.Adc[j], event.Adc[i]);
                        }

                        if (event.Adc[i] > 120 && event.Adc[j] > 120) {
                            for (size_t k = 0; k < event.Size(); ++k) {
                                if (event.Mod[k] == 3 && event.Ch[k] > 7) {
                                    const double dT_sect_cdte = static_cast<double>(event.Ts[j]) - static_cast<double>(event.Ts[k]);
                                    hSect_CdTe_dT->Fill(dT_sect_cdte);
                                    hSect_CdTe_dT_ADC->Fill(dT_sect_cdte, event.Adc[k]);
                                }
                                if (event.Mod[k] == 4) {
                                    const double dT_sect_hpge = static_cast<double>(event.Ts[j]) - static_cast<double>(event.Ts[k]);
                                    hSect_HPGe_dT->Fill(dT_sect_hpge);
                                    hSect_HPGe_dT_ADC->Fill(dT_sect_hpge, event.Adc[k]);
                                }
                            }
                        }
                    }
                }
                if (event.Mod[j] == 1 && j != i) {
                    hRingRing->Fill(event.Ch[i], event.Ch[j]);
                }
            }
        }
        if (event.Mod[i] == 2) {
            for (size_t j = 0; j < event.Size(); ++j) {
                if (event.Mod[j] == 2 && i != j) {
                    hSectSect->Fill(event.Ch[i], event.Ch[j]);
                }
            }
        }
        if (event.Mod[i] == 3) {
            mod3_ch_adc->Fill(event.Ch[i], event.Adc[i]);
        }
        if (event.Mod[i] == 4) {
            mod4_ch_adc->Fill(event.Ch[i], event.Adc[i]);
        }
    }
}

void FillHistogramsFromEventTree(TTree* tree,
                                 ThreadedHistogramSet& histograms,
                                 unsigned int nthreads)
{
    if (!tree) {
        return;
    }

    ROOT::EnableImplicitMT(nthreads);
    ROOT::TTreeProcessorMT processor(*tree, nthreads);

    processor.Process([&histograms](TTreeReader& reader) {
        TTreeReaderValue<std::vector<UShort_t>> ts(reader, "Ts");
        TTreeReaderValue<std::vector<UShort_t>> mod(reader, "Mod");
        TTreeReaderValue<std::vector<UShort_t>> ch(reader, "Ch");
        TTreeReaderValue<std::vector<UShort_t>> adc(reader, "Adc");

        while (reader.Next()) {
            const BuiltEventView event{*ts, *mod, *ch, *adc};
            FillHistograms(histograms, event);
        }
    });
}

void FillHistogramsFromBuiltEventQueue(ThreadSafeQueue<BuiltEvent>& queue,
                                       ThreadedHistogramSet& histograms,
                                       unsigned int nthreads)
{
    unsigned int workerCount = nthreads;
    if (workerCount == 0) {
        workerCount = std::thread::hardware_concurrency();
    }
    if (workerCount == 0) {
        workerCount = 1;
    }

    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    for (unsigned int i = 0; i < workerCount; ++i) {
        workers.push_back(std::thread([&queue, &histograms]() {
            BuiltEvent event;
            while (queue.pop(event)) {
                FillHistograms(histograms, MakeBuiltEventView(event));
                event.Clear();
            }
        }));
    }

    for (size_t i = 0; i < workers.size(); ++i) {
        workers[i].join();
    }
}
