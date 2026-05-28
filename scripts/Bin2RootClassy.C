/*****************************************************
 * macro: Bin2RootClassy.C                           *
 * Read binary file from TechnoAP DAQ                *
 *   Version 1.2    10/4/2026   J.Smallcombe         *
 *                                                   *
 * Usage:                                            *
 * root -l 'Bin2RootClassy.C++O("/Path/Run1")'       *
 * or                                                *
 * .L Bin2RootClassy.C++O                            *
 * Bin2RootClassy("/Path/Run1");                     *
    * -> Read Run1_000000.bin  etc                   *
    * -> Write /Path/Run1_out.root                   *
 * -> Write /Path/Run1_out_1.root  if >2GB           *
 *                                                   *
 *  void Bin2RootClassy(RunName,OutputName,          *
 *                  ReadChunk,TimeJitterTollerance)  *
 *                                                   *
 *  ReadChunk<0 entire file read, no sorting         *
 *****************************************************/


#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TSystem.h>
#include <TStopwatch.h>
#include <TParameter.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

// =========================
// Event structure
// =========================
struct Event {
    Long64_t ts = -1;
    UShort_t mod = 0;
    UShort_t ch = 0;
    UShort_t adc = 0;

    // Future expansion
    UShort_t neg = 0;
    UShort_t unit = 0;
    UShort_t wavFlag = 0;
    std::vector<UShort_t> waveform;
};

// =========================
// Base digitiser
// =========================
class DigitiserBase {
protected:
    TString run;
    int mod;
    int fileIndex = 0;
    bool isOpen = false;
    bool isFinished = false;
    std::ifstream file;

    Long64_t lastTs = -1;
    Long64_t disorderCount = 0;

    static Long64_t TS_TOLERANCE; // ns
    
public:
    static void SetTsTolerance(Long64_t ts){TS_TOLERANCE=ts;}

    DigitiserBase(TString runName, int module)
        : run(runName), mod(module) {}

    virtual ~DigitiserBase() {
        if (file.is_open()) file.close();
    }

    virtual TString buildFileName() = 0;
    virtual bool decode(UShort_t* buf, Event& ev) = 0;

    Long64_t getLastTs() const { return lastTs; }

    bool openNextFile() {
        if (file.is_open()) file.close();

        std::cout<<"opening file for module "<<mod<<std::endl;
        TString fname = buildFileName();
        file.open(fname.Data(), std::ios::in | std::ios::binary);

        if (!file) {
            isOpen = false;
            std::cout<<"Failed to open file "<<fname<<std::endl;
            isFinished = true; 
            return false;
        }

        std::cout << "Opened: " << fname << std::endl;

        isOpen = true;
        fileIndex++;
        return true;
    }

    bool getNextEvent(Event& ev) {
        UShort_t ReadBuf[5];
        
        if(isFinished)return false;

        while (true) {
            if (!isOpen) {
                if (!openNextFile()) return false;
            }

            file.read((char*)&ReadBuf, sizeof(ReadBuf));

            if (file.fail()) {
                isOpen = false;
                continue;
            }

            UShort_t Dbuf[5];
            for (int i = 0; i < 5; i++)
                Dbuf[i] = ((ReadBuf[i] & 0xFF) << 8) | ((ReadBuf[i] >> 8) & 0xFF);

            ev.mod = mod;
            if (!decode(Dbuf, ev)) continue;

            // --- Disorder check ---
            if (lastTs >= 0 && ev.ts < lastTs - TS_TOLERANCE) {

                disorderCount++;

                if (disorderCount < 10 || disorderCount % 1000 == 0) {
                    std::cout << "[TS DISORDER] Digi " << mod
                              << " prev=" << lastTs
                              << " new=" << ev.ts
                              << " dT=" << (ev.ts - lastTs)
                              << std::endl;
                }

                // Only update if not catastrophic
                if (ev.ts > lastTs)
                    lastTs = ev.ts;
            } else {
                lastTs = ev.ts;
            }

            return true;
        }
    }
};
Long64_t DigitiserBase::TS_TOLERANCE = 100000; // ns

// =========================
// APV8104 (Mod 0)
// =========================
class APV8104 : public DigitiserBase {
public:
    APV8104(TString runName,int module) : DigitiserBase(runName, module) {}

    static TString buildFileName(TString rn,int filei) {
        TString name;
        name.Form("%s_%06d.bin", rn.Data(), filei);
        return name;
    }
    TString buildFileName() override {
        return buildFileName(run,fileIndex);
    }

    bool decode(UShort_t* Dbuf, Event& ev) override {
        ev.ts  = ((Long64_t)(Dbuf[0] & 0xFFFF) << 20);
        ev.ts  = ev.ts << 20;
        ev.ts += ((Long64_t)(Dbuf[1] & 0xFFFF) << 24);
        ev.ts += ((Long64_t)(Dbuf[2] & 0xFFFF) << 8);
        ev.ts += ((Long64_t)(Dbuf[3] & 0xFF00) >> 8);

        ev.ch  = (Dbuf[4] & 0xE000) >> 13;
        ev.adc = (Dbuf[4] & 0x1FFF);

        return true;
    }
};

// =========================
// APV8032 (Mod 1–3)
// =========================
class APV8032 : public DigitiserBase {
public:
    UShort_t fmod = 0;
    APV8032(TString runName,int module, int filemod)
        : DigitiserBase(runName, module),fmod(filemod) {}

    static TString buildFileName(TString rn,int fmodl,int filei) {
        TString name;
        if(filei){
            name.Form("%s_000000_%d-%011d.bin",rn.Data(), fmodl, filei);
        }else{ 
            name.Form("%s_000000_%d.bin", rn.Data(), fmodl);
        }
        return name;
    }
    TString buildFileName() override {
        return buildFileName(run,fmod,fileIndex);
    }

    bool decode(UShort_t* Dbuf, Event& ev) override {
        ev.ts  = ((Long64_t)(Dbuf[0] & 0xFFFF) << 20);
        ev.ts  = ev.ts << 12;
        ev.ts += ((Long64_t)(Dbuf[1] & 0xFFFF) << 16);
        ev.ts += ((Long64_t)(Dbuf[2]));
        ev.ts *= 10;

        ev.ch  = (Dbuf[3] & 0x001F);
        ev.adc = (Dbuf[4] & 0x1FFF);

        return true;
    }
};

// =========================
// APV8016A (Mod 4)
// =========================
class APV8016A : public DigitiserBase {
public:
    UShort_t fmod = 0;
    APV8016A(TString runName,int module, int filemod)
        : DigitiserBase(runName, module),fmod(filemod) {}

    static TString buildFileName(TString rn,int fmodl,int filei) {
        TString name;
        if(filei){
            name.Form("%s_Ge_000000_%d-%011d.bin",rn.Data(), fmodl, filei);
        }else{ 
            name.Form("%s_Ge_000000_%d.bin", rn.Data(), fmodl);
        }
        return name;
    }
    TString buildFileName() override {
        return buildFileName(run,fmod,fileIndex);
    }

    bool decode(UShort_t* Dbuf, Event& ev) override {
        ev.ts  = ((Long64_t)(Dbuf[0] & 0xFFFF) << 20);
        ev.ts  = ev.ts << 12;
        ev.ts += ((Long64_t)(Dbuf[1] & 0xFFFF) << 16);
        ev.ts += ((Long64_t)(Dbuf[2]));
        ev.ts *= 10;

        ev.ch  = (Dbuf[3] & 0x000F);
        ev.adc = (Dbuf[4] & 0x3FFF);

        return true;
    }
};

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



std::vector<std::unique_ptr<DigitiserBase>>
BuildDigitiserList(const TString& runName)
{
    std::cout<<"Building digitiser list from files."<<std::endl;
    std::vector<std::unique_ptr<DigitiserBase>> digitisers;
    
    //APV8104
    // Check existence (kFALSE = exists and accessible)
    if (!gSystem->AccessPathName(APV8104::buildFileName(runName, 0))) {
        // Construct only if file exists
        std::cout<<"Added APV8104 as module "<<digitisers.size()<<std::endl;
        digitisers.push_back(std::make_unique<APV8104>(runName,digitisers.size()));
    }
    //APV8032
    for (int fmodl = 1; fmodl <= 5; ++fmodl) {
        // Check existence (kFALSE = exists and accessible)
        if (!gSystem->AccessPathName(APV8032::buildFileName(runName, fmodl, 0))) {
            // Construct only if file exists
            std::cout<<"Added APV8032_"<<fmodl<<" as module "<<digitisers.size()<<std::endl;
            digitisers.push_back(std::make_unique<APV8032>(runName,digitisers.size(), fmodl));
        }
    }
    //APV8016A
    for (int fmodl = 1; fmodl <= 5; ++fmodl) {
        // Check existence (kFALSE = exists and accessible)
        if (!gSystem->AccessPathName(APV8016A::buildFileName(runName, fmodl, 0))) {
            // Construct only if file exists
            std::cout<<"Added APV8016A_"<<fmodl<<" as module "<<digitisers.size()<<std::endl;
            digitisers.push_back(std::make_unique<APV8016A>(runName,digitisers.size(),fmodl));
        }
    }
    
    return digitisers;
}

// =========================
// Main
// =========================
void Bin2RootClassy(TString run,TString out="",int CHUNK_SIZE_IN = 100,Long64_t TS_TOLERANCE = 100000)
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
    cout << Form("\n RealTime = %d seconds, CpuTime = %d seconds\n\n",(Int_t)rtime,(Int_t)ctime );
}
