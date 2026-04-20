#ifndef TechnoAPDigitisers
#define TechnoAPDigitisers


#include <TString.h>
#include <TSystem.h>
#include <TDirectory.h>
#include <TH1F.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>


#include <Globals.h>
#include <BuiltEvent.h>


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

    virtual int channels(){ return 1; }
    virtual std::string name(){ return "-"; }

    Long64_t getLastTs() const { return lastTs; }
    int getModule() const { return mod; }

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

// =========================
// APV8104 (Mod 0)
// =========================
class APV8104 : public DigitiserBase {
public:
    APV8104(TString runName,int module) : DigitiserBase(runName, module) {}
    static int ModuleZeroIndex;
    std::string name()override{ return "APV8104"; }

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
// APV8032 
// =========================
class APV8032 : public DigitiserBase {
public:
    UShort_t fmod = 0;
    APV8032(TString runName,int module, int filemod)
        : DigitiserBase(runName, module),fmod(filemod) {}
    static int ModuleZeroIndex;
    int channels()override{ return 32; }
    std::string name()override{ return "APV8032"; }

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
// APV8016A 
// =========================
class APV8016A : public DigitiserBase {
public:
    UShort_t fmod = 0;
    APV8016A(TString runName,int module, int filemod)
        : DigitiserBase(runName, module),fmod(filemod) {}
    static int ModuleZeroIndex;
    int channels()override{ return 16; }
    std::string name()override{ return "APV8016A"; }

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



struct DigitiserAdcHistograms {
    std::vector<TH1F*> histograms;

    DigitiserAdcHistograms() = default;
    DigitiserAdcHistograms(const DigitiserAdcHistograms&) = delete;
    DigitiserAdcHistograms& operator=(const DigitiserAdcHistograms&) = delete;
    DigitiserAdcHistograms(DigitiserAdcHistograms&& other) noexcept;
    DigitiserAdcHistograms& operator=(DigitiserAdcHistograms&& other) noexcept;
    ~DigitiserAdcHistograms();

    void Fill(int module, int channel, double value);
    void SetDirectory(TDirectory* directory);
    Int_t Write(const char* name = nullptr, Int_t option = 0, Int_t bufsize = 0);

private:
    std::vector<int> moduleOffsets;
    std::vector<int> moduleChannels;
    std::vector<bool> ownedByThis;

    void Clear();

    friend DigitiserAdcHistograms BuildDigitiserAdcHistograms(const std::vector<std::unique_ptr<DigitiserBase>>& digitisers);
};

std::vector<std::unique_ptr<DigitiserBase>>  BuildDigitiserList(const TString& runName);
DigitiserAdcHistograms BuildDigitiserAdcHistograms(const std::vector<std::unique_ptr<DigitiserBase>>& digitisers);

#endif
