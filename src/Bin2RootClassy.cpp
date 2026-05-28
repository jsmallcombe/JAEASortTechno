#include <Bin2RootClassy.h>

// =========================
// Global chunk reader
// =========================
bool readChunk(DigitiserBase& digi,
               int chunkSize,
               TTree* tree,
               Event& ev)
{
    int count = 0;
    
    bool DoChunking=(chunkSize > 0);

    while (digi.getNextEvent(ev)) {
        tree->Fill();
        count++;
        
        if(DoChunking)
            if(count >= chunkSize)
                    break;
    }

    return count > 0;
}


// =========================
// Main
// =========================
void Bin2RootClassy(TString run,TString out,int CHUNK_SIZE_IN,Long64_t TS_TOLERANCE)
{
    DigitiserBase::SetTsTolerance(TS_TOLERANCE);

    const int CHUNK_SIZE = CHUNK_SIZE_IN;
    
    // Start timer
    TStopwatch timer;
    timer.Start();
    
    if(!out.Length())out=run + "_out.root";
    TFile *fOut = new TFile(out, "RECREATE");
    TTree *fOutT = new TTree("OutTTree01", run);

    // ROOT auto file splitting (~1.9 GB safe)
    fOutT->SetMaxTreeSize(1900LL * 1024 * 1024);
    // Removes autosave in the case of crash, but removes duplicate TTree keys
    fOutT->SetAutoSave(0);
    
    auto p = new TParameter<int>("ChunkSize", CHUNK_SIZE_IN);
    fOutT->GetUserInfo()->Add(p);

    Event ev;

    // Direct branch binding
    fOutT->Branch("Ts",  &ev.ts);
    fOutT->Branch("Mod", &ev.mod);
    fOutT->Branch("Ch",  &ev.ch);
    fOutT->Branch("Adc", &ev.adc);

    // Create digitisers
//     std::vector<std::unique_ptr<DigitiserBase>> digitisers;
// // //     digitisers.push_back(std::make_unique<APV8104>(run));
//     digitisers.push_back(std::make_unique<APV8032>(run,0,1));
//     digitisers.push_back(std::make_unique<APV8032>(run,1,2));
//     digitisers.push_back(std::make_unique<APV8016A>(run,2,1));
//     digitisers.push_back(std::make_unique<APV8016A>(run,3,2));
//     First number is the user defined global module number for output tree
//     Second number is the DAQ module index for the input files
    
// Downside of this function, the module numbers will shift in runs that are missing particular digitisers
// Create some kind of global digitiser number index that can be set externally/manually 
std::vector<std::unique_ptr<DigitiserBase>> digitisers=BuildDigitiserList(run);
    
    
    Long64_t globalMaxTs = -1;
    bool anyActive = true;

    while (anyActive) {
        anyActive = false;

        for (auto& digiPtr : digitisers) {

            auto& digi = *digiPtr;

            bool accepted = false;

            while (!accepted) {

                if (!readChunk(digi, CHUNK_SIZE, fOutT, ev)) {
                    break;
                }

                anyActive = true;
                Long64_t digiTs = digi.getLastTs();

                if (globalMaxTs < 0 || digiTs >= globalMaxTs) {
                    globalMaxTs = digiTs;
                    accepted = true;
                }
                // else → read another chunk
            }
        }
    }
    // Get current tree file BEFORE closing anything
    TFile* currentFile = fOutT->GetCurrentFile();

    // Write everything
    if (currentFile) {
        currentFile->Write("", TObject::kOverwrite);
        if (currentFile != fOut) {
            currentFile->Close();
        }
    }

    // Only write fOut if it's different
    if (fOut && fOut != currentFile) {
        fOut->Write("", TObject::kOverwrite);
        fOut->Close();
    }

    std::cout << "Finished processing " << run << std::endl;
    
    timer.Stop();
    Double_t rtime = timer.RealTime();
    Double_t ctime = timer.CpuTime();
    std::cout << Form("\n RealTime = %d seconds, CpuTime = %d seconds\n\n",(Int_t)rtime,(Int_t)ctime );
}
