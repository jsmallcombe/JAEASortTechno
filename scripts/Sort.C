#define Sort_cxx
#include "Sort.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>

void Sort::Loop(const char *outfilename="Histograms.root")
{
//   In a ROOT session, you can do:
//      root> .L Sort.C
//      root> Sort t
//      root> t.GetEntry(12); // Fill t data members with entry number 12
//      root> t.Show();       // Show values of entry 12
//      root> t.Show(16);     // Read and show values of entry 16
//      root> t.Loop();       // Loop on all entries
//

//     This is the loop skeleton where:
//    jentry is the global entry number in the chain
//    ientry is the entry number in the current Tree
//  Note that the argument to GetEntry must be:
//    jentry for TChain::GetEntry
//    ientry for TTree::GetEntry and TBranch::GetEntry
//
//       To read only selected branches, Insert statements like:
// METHOD1:
//    fChain->SetBranchStatus("*",0);  // disable all branches
//    fChain->SetBranchStatus("branchname",1);  // activate branchname
// METHOD2: replace line
//    fChain->GetEntry(jentry);       //read all branches
//by  b_branchname->GetEntry(ientry); //read only this branch
   if (fChain == 0) return;

   TH2 * siall = new TH2F("siall","rings vs sectors",32,0,32,32,0,32);
   TH1 * sidt = new TH1F("sid","rings and sectors time differences",200,-1000,1000);
   TH2 * ring_sector_E = new TH2F("ring_sector_E","ring E vs sector E",512,0,8192,512,0,8192);
   TH2 * ring_sector_E_reduced = new TH2F("ring_sector_E_reduced","ring E vs sector E",512,0,8192,512,0,8192);
   TH2 * pmpt_ring_sector_E = new TH2F("pmpt_ring_sector_E","pmpt_ring E vs sector E *Tdiff<100  " ,512,0,8192,512,0,8192);
   TH2 * pmpt_ring_sector_E_reduced = new TH2F("pmpt_ring_sector_E_reduced","pmpt_ring E vs sector E *Tdiff<100  " ,512,0,8192,512,0,8192);

   TH1	* hSect_CdTe_dT = new TH1F("Sect_CdTe_dT","Sector - CdTe time difference",400,-2000,2000);
   TH1	* hSect_HPGe_dT = new TH1F("Sect_HPGe_dT","Sector - HPGe time difference",400,-2000,2000);

   TH2	* hSect_CdTe_dT_ADC = new TH2F("Sect_CdTe_dT_ADC","Sector - CdTe time difference vs ADC",400,-2000,2000,1024,0,8192);
   TH2	* hSect_HPGe_dT_ADC = new TH2F("Sect_HPGe_dT_ADC","Sector - HPGe time difference vs ADC",400,-2000,2000,1024,0,8192);

   TH2F *hRingRing = new TH2F("RingRing","Ring # vs Ring #",32,0,32,32,0,32);
   TH2F *hSectSect = new TH2F("SectSect","Sect # vs Sect #",32,0,32,32,0,32);

   TH1 *hSectE_divRingE = new TH1F("SectE_divRingE","Sector energy divided by ring energy",1000,0,10);

   TH2 *mod1_ch_adc = new TH2F("mod1_ch_adc","Module 1 channel vs ADC",32,0,32,1024,0,8192);
   TH2 *mod2_ch_adc = new TH2F("mod2_ch_adc","Module 2 channel vs ADC",32,0,32,1024,0,8192);
   TH2 *mod3_ch_adc = new TH2F("mod3_ch_adc","Module 3 channel vs ADC",32,0,32,1024,0,8192);
   TH2 *mod4_ch_adc = new TH2F("mod4_ch_adc","Module 4 channel vs ADC",32,0,32,1024,0,8192);
   
   Long64_t nentries = fChain->GetEntriesFast();

   Long64_t nbytes = 0, nb = 0;
   for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);   nbytes += nb;
      // if (Cut(ientry) < 0) continue;
      for(size_t i=0;i<Mod->size();i++){
	if(Mod->at(i) == 2)
	  mod1_ch_adc->Fill(Ch->at(i),Adc->at(i));
	if(Mod->at(i) == 1){ // Rings
	  mod2_ch_adc->Fill(Ch->at(i),Adc->at(i));
	  for(size_t j=0;j<Mod->size();j++){
	    if(Mod->at(j) == 2){ // Sectors
	      siall->Fill(Ch->at(j),Ch->at(i));
	      ring_sector_E->Fill(Adc->at(j),Adc->at(i));
	      if(Ch->at(i)!=11&&Ch->at(i)!=16&&Ch->at(i)!=17&&Ch->at(i)!=18){
		ring_sector_E_reduced->Fill(Adc->at(j),Adc->at(i));
	      sidt->Fill(Ts->at(i)-Ts->at(j));
	      }
	      if((Ts->at(i)-Ts->at(j))>-100&&(Ts->at(i)-Ts->at(j))<100){
		pmpt_ring_sector_E->Fill(Adc->at(j),Adc->at(i));
		if(Ch->at(i)!=11&&Ch->at(i)!=16&&Ch->at(i)!=17&&Ch->at(i)!=18){
		pmpt_ring_sector_E_reduced->Fill(Adc->at(j),Adc->at(i));
		}
		  

		hSectE_divRingE->Fill((double)Adc->at(j)/(double)Adc->at(i));
	
		if(Adc->at(i) > 120 && Adc->at(j) > 120){
			for(size_t k=0;k<Mod->size();k++){
				if(Mod->at(k) == 3 && Ch->at(k) > 7){ // CdTe
					double dT_sect_cdte = Ts->at(j) - Ts->at(k);
					hSect_CdTe_dT->Fill(dT_sect_cdte);
					hSect_CdTe_dT_ADC->Fill(dT_sect_cdte,Adc->at(k));
				
				}
				if(Mod->at(k) == 4){ // HPGe 
					double dT_sect_hpge = Ts->at(j) - Ts->at(k);
					hSect_HPGe_dT->Fill(dT_sect_hpge);
					hSect_HPGe_dT_ADC->Fill(dT_sect_hpge,Adc->at(k));
				}
			}
		}

	      }
	      
	    }
	    if(Mod->at(j) == 1 && j!=i){
		hRingRing->Fill(Ch->at(i),Ch->at(j));
	    }
	  }
	  
	}
	if(Mod->at(i) == 2){ // Sectors
		for(size_t j=0;j<Mod->size();j++){
			if(Mod->at(j) == 2 && i!=j){
				hSectSect->Fill(Ch->at(i),Ch->at(j));
			}
		}
	}
	if(Mod->at(i) == 3)
	  mod3_ch_adc->Fill(Ch->at(i),Adc->at(i));
	if(Mod->at(i) == 4)
	  mod4_ch_adc->Fill(Ch->at(i),Adc->at(i));
      }
   }
   TFile *outfile = new TFile(outfilename,"RECREATE");
   siall->Write();
   mod1_ch_adc->Write();
   mod2_ch_adc->Write();
   mod3_ch_adc->Write();
   mod4_ch_adc->Write();
   sidt->Write();
   ring_sector_E->Write();
   pmpt_ring_sector_E->Write();
   ring_sector_E_reduced->Write();
   pmpt_ring_sector_E_reduced->Write();
	hSect_CdTe_dT->Write();
	hSect_HPGe_dT->Write();
	hRingRing->Write();
	hSectSect->Write();
	hSect_CdTe_dT_ADC->Write();
	hSect_HPGe_dT_ADC->Write();
	hSectE_divRingE->Write();
   outfile->Write();
   outfile->Close();		// 
      
}
