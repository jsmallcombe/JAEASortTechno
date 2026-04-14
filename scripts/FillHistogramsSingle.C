#include <TDirectory.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2D.h>
#include <TH2F.h>
#include <TH3F.h>
#include <TStopwatch.h>
#include <TString.h>
#include <TTree.h>

#include <iostream>
#include <vector>

void FillHistogramsSingle(const char* infilename, const char* outfilename = "")
{
    TStopwatch timer;
    timer.Start();

    TFile* infile = TFile::Open(infilename, "READ");
    if (!infile || infile->IsZombie()) {
        std::cout << "Error: Could not open input file\n";
        return;
    }

    TTree* intree = dynamic_cast<TTree*>(infile->Get("EventTree"));
    if (!intree) {
        std::cout << "Error: Could not find TTree 'EventTree' in input file\n";
        infile->Close();
        return;
    }

    TString outname = outfilename;
    if (!outname.Length()) {
        outname = infilename;
        if (outname.EndsWith(".root")) {
            outname.ReplaceAll(".root", "_hist.root");
        } else {
            outname += "_hist.root";
        }
    }

    std::vector<UShort_t>* Ts = 0;
    std::vector<UShort_t>* Mod = 0;
    std::vector<UShort_t>* Ch = 0;
    std::vector<UShort_t>* Adc = 0;

    intree->SetBranchAddress("Ts", &Ts);
    intree->SetBranchAddress("Mod", &Mod);
    intree->SetBranchAddress("Ch", &Ch);
    intree->SetBranchAddress("Adc", &Adc);

    TH2F* siall = new TH2F("siall", "rings vs sectors", 32, 0, 32, 32, 0, 32);
    TH1F* sidt = new TH1F("sid", "rings and sectors time differences", 200, -1000, 1000);
    TH2F* ring_sector_E = new TH2F("ring_sector_E", "ring E vs sector E", 512, 0, 8192, 512, 0, 8192);
    TH2F* ring_sector_E_reduced = new TH2F("ring_sector_E_reduced", "ring E vs sector E", 512, 0, 8192, 512, 0, 8192);
    TH2F* pmpt_ring_sector_E = new TH2F("pmpt_ring_sector_E", "pmpt_ring E vs sector E *Tdiff<100", 512, 0, 8192, 512, 0, 8192);
    TH2F* pmpt_ring_sector_E_reduced = new TH2F("pmpt_ring_sector_E_reduced", "pmpt_ring E vs sector E *Tdiff<100", 512, 0, 8192, 512, 0, 8192);
    TH1F* hSect_CdTe_dT = new TH1F("Sect_CdTe_dT", "Sector - CdTe time difference", 400, -2000, 2000);
    TH1F* hSect_HPGe_dT = new TH1F("Sect_HPGe_dT", "Sector - HPGe time difference", 400, -2000, 2000);
    TH2F* hSect_CdTe_dT_ADC = new TH2F("Sect_CdTe_dT_ADC", "Sector - CdTe time difference vs ADC", 400, -2000, 2000, 1024, 0, 8192);
    TH2F* hSect_HPGe_dT_ADC = new TH2F("Sect_HPGe_dT_ADC", "Sector - HPGe time difference vs ADC", 400, -2000, 2000, 1024, 0, 8192);
    TH2F* hRingRing = new TH2F("RingRing", "Ring # vs Ring #", 32, 0, 32, 32, 0, 32);
    TH2F* hSectSect = new TH2F("SectSect", "Sect # vs Sect #", 32, 0, 32, 32, 0, 32);
    TH1F* hSectE_divRingE = new TH1F("SectE_divRingE", "Sector energy divided by ring energy", 1000, 0, 10);
    TH2F* mod1_ch_adc = new TH2F("mod1_ch_adc", "Module 1 channel vs ADC", 32, 0, 32, 1024, 0, 8192);
    TH2F* mod2_ch_adc = new TH2F("mod2_ch_adc", "Module 2 channel vs ADC", 32, 0, 32, 1024, 0, 8192);
    TH2F* mod3_ch_adc = new TH2F("mod3_ch_adc", "Module 3 channel vs ADC", 32, 0, 32, 1024, 0, 8192);
    TH2F* mod4_ch_adc = new TH2F("mod4_ch_adc", "Module 4 channel vs ADC", 32, 0, 32, 1024, 0, 8192);
    TH3F* mod_ch_adc_ts = new TH3F("mod_ch_adc_ts", "Module vs channel vs ADC", 8, 0, 8, 32, 0, 32, 256, 0, 8192);
    TH2D* sector_ring_energy_double = new TH2D("sector_ring_energy_double", "Sector E vs Ring E", 512, 0, 8192, 512, 0, 8192);
    TH2F* ESumPart[4];
    for (int i = 0; i < 4; ++i) {
        ESumPart[i] = new TH2F(Form("ESumPart_%d", i),
                               Form("E%d vs Esum;Esum;E%d", i, i),
                               500, 0, 2000, 500, 0, 2000);
    }

    Long64_t nentries = intree->GetEntries();
    for (Long64_t entry = 0; entry < nentries; ++entry) {
        intree->GetEntry(entry);

        for (size_t i = 0; i < Mod->size(); ++i) {
            mod_ch_adc_ts->Fill(Mod->at(i), Ch->at(i), Adc->at(i));

            if (Mod->at(i) == 2) {
                mod1_ch_adc->Fill(Ch->at(i), Adc->at(i));
            }
            if (Mod->at(i) == 1) {
                mod2_ch_adc->Fill(Ch->at(i), Adc->at(i));
                for (size_t j = 0; j < Mod->size(); ++j) {
                    if (Mod->at(j) == 2) {
                        double dT = static_cast<double>(Ts->at(i)) - static_cast<double>(Ts->at(j));
                        siall->Fill(Ch->at(j), Ch->at(i));
                        ring_sector_E->Fill(Adc->at(j), Adc->at(i));
                        sector_ring_energy_double->Fill(static_cast<double>(Adc->at(j)), static_cast<double>(Adc->at(i)));

                        if (Ch->at(i) != 11 && Ch->at(i) != 16 && Ch->at(i) != 17 && Ch->at(i) != 18) {
                            ring_sector_E_reduced->Fill(Adc->at(j), Adc->at(i));
                            sidt->Fill(dT);
                        }

                        if (dT > -100.0 && dT < 100.0) {
                            pmpt_ring_sector_E->Fill(Adc->at(j), Adc->at(i));
                            if (Ch->at(i) != 11 && Ch->at(i) != 16 && Ch->at(i) != 17 && Ch->at(i) != 18) {
                                pmpt_ring_sector_E_reduced->Fill(Adc->at(j), Adc->at(i));
                            }

                            if (Adc->at(i) != 0) {
                                hSectE_divRingE->Fill(static_cast<double>(Adc->at(j)) / static_cast<double>(Adc->at(i)));
                            }

                            if (Ch->at(i) < 4) {
                                ESumPart[Ch->at(i)]->Fill(Adc->at(i) + Adc->at(j), Adc->at(i));
                            }

                            if (Adc->at(i) > 120 && Adc->at(j) > 120) {
                                for (size_t k = 0; k < Mod->size(); ++k) {
                                    if (Mod->at(k) == 3 && Ch->at(k) > 7) {
                                        double dT_sect_cdte = static_cast<double>(Ts->at(j)) - static_cast<double>(Ts->at(k));
                                        hSect_CdTe_dT->Fill(dT_sect_cdte);
                                        hSect_CdTe_dT_ADC->Fill(dT_sect_cdte, Adc->at(k));
                                    }
                                    if (Mod->at(k) == 4) {
                                        double dT_sect_hpge = static_cast<double>(Ts->at(j)) - static_cast<double>(Ts->at(k));
                                        hSect_HPGe_dT->Fill(dT_sect_hpge);
                                        hSect_HPGe_dT_ADC->Fill(dT_sect_hpge, Adc->at(k));
                                    }
                                }
                            }
                        }
                    }

                    if (Mod->at(j) == 1 && j != i) {
                        hRingRing->Fill(Ch->at(i), Ch->at(j));
                    }
                }
            }

            if (Mod->at(i) == 2) {
                for (size_t j = 0; j < Mod->size(); ++j) {
                    if (Mod->at(j) == 2 && i != j) {
                        hSectSect->Fill(Ch->at(i), Ch->at(j));
                    }
                }
            }

            if (Mod->at(i) == 3) {
                mod3_ch_adc->Fill(Ch->at(i), Adc->at(i));
            }
            if (Mod->at(i) == 4) {
                mod4_ch_adc->Fill(Ch->at(i), Adc->at(i));
            }
        }
    }

    TFile* outfile = TFile::Open(outname, "RECREATE");
    if (!outfile || outfile->IsZombie()) {
        std::cout << "Error: Could not create output file\n";
        infile->Close();
        return;
    }

    TDirectory* timing = outfile->mkdir("timing");
    TDirectory* correlations = outfile->mkdir("correlations");
    TDirectory* modules = outfile->mkdir("modules");
    TDirectory* grouped = outfile->mkdir("grouped");

    timing->cd();
    sidt->Write();
    hSect_CdTe_dT->Write();
    hSect_HPGe_dT->Write();
    hSect_CdTe_dT_ADC->Write();
    hSect_HPGe_dT_ADC->Write();

    correlations->cd();
    siall->Write();
    ring_sector_E->Write();
    ring_sector_E_reduced->Write();
    pmpt_ring_sector_E->Write();
    pmpt_ring_sector_E_reduced->Write();
    hRingRing->Write();
    hSectSect->Write();
    hSectE_divRingE->Write();
    sector_ring_energy_double->Write();

    modules->cd();
    mod1_ch_adc->Write();
    mod2_ch_adc->Write();
    mod3_ch_adc->Write();
    mod4_ch_adc->Write();
    mod_ch_adc_ts->Write();

    grouped->cd();
    for (int i = 0; i < 4; ++i) {
        ESumPart[i]->Write();
    }

    outfile->Write();
    outfile->Close();
    infile->Close();

    std::cout << "\nDone\n";

    timer.Stop();
    Double_t rtime = timer.RealTime();
    Double_t ctime = timer.CpuTime();
    std::cout << Form("\n RealTime = %d seconds, CpuTime = %d seconds\n\n",(Int_t)rtime,(Int_t)ctime );

    std::cout << "Wrote histograms to " << outname << "\n";
}
