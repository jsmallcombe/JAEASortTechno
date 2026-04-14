#ifndef JAEASortDetectors
#define JAEASortDetectors

#include <TVector3.h>
#include <array>
#include <vector>


using namespace std;

struct ChannelCalibration {
    Double_t p0, p1, p2;
    UShort_t DetectorType, Index;
    Double_t TOff;

    // Default constructor with default parameter values
    ChannelCalibration(double p0Val = 0.0, double p1Val = 1.0, double p2Val = 0.0, int detectorTypeVal = 0, int indexVal = 0, UShort_t toff = 0)
        : p0(p0Val), p1(p1Val), p2(p2Val), DetectorType(detectorTypeVal), Index(indexVal) , TOff(toff){}
};
    
class JAEASortIO;

class DetHit{
    
    friend class JAEASortIO;
    
    private :
    static vector<vector<ChannelCalibration>> ChanCal;
    
    static void ExpandCal(UShort_t Mod,UShort_t Chan);
    static const ChannelCalibration& FindCalibration(UShort_t Mod,UShort_t Chan);
    
    public:
        
    enum DetType{HPGe,LaBr,SiDeltaE,Si,SiDeltaE_B,Si_B,Solar,Dice,CdTe};
    
    static void SetCalibrationParam(UShort_t Mod,UShort_t Chan,double Pol0=0, double Pol1=1,double Pol2=0);
    static array<double,3> GetCal(UShort_t Mod,UShort_t Chan);
    static void SetCalibrationDirect(UShort_t DetT,UShort_t Index,double Pol0=0, double Pol1=1,double Pol2=0);
    
    static UShort_t GetDetType(UShort_t Mod,UShort_t Chan);
    static UShort_t GetIndex(UShort_t Mod,UShort_t Chan);
    static Double_t GetTOff(UShort_t Mod,UShort_t Chan);
    
    static void SetDetType(UShort_t Mod,UShort_t Chan,UShort_t DetT);
    static void SetIndex(UShort_t Mod,UShort_t Chan,UShort_t Index);
    static void SetTOff(UShort_t Mod,UShort_t Chan,Double_t toff);
    
    
    private :
        
    Double_t fTimeStamp;
    UShort_t fCharge;
    UShort_t fModule;
    UShort_t fChannel;
    
    bool fEnergySet;
    mutable Double_t fEnergy{0.};    //!<! Energy of the Hit.
    

    public:
    
    DetHit(){}; 
    DetHit(Double_t tTS,UShort_t tC, UShort_t tMod, UShort_t tChan)
    : fTimeStamp(tTS), fCharge(tC), fModule(tMod), fChannel(tChan), fEnergySet(false), fEnergy(0.0) {};
    virtual ~DetHit(){};
    
    DetHit(const DetHit& other) {
        *this = other;  // Reuse the logic of the copy assignment operator
    };
    
    DetHit& operator=(const DetHit& other) {
        if (this != &other) {  // self-assignment check
            fTimeStamp = other.fTimeStamp;
            fCharge = other.fCharge;
            fModule = other.fModule;
            fChannel = other.fChannel;
            fEnergySet = false; // in case calibration has been updated and DetHit is being read from an old tree
            fEnergy = 0;
        }
        return *this;
    };
    
    Double_t Energy();
    Double_t Time() const {return fTimeStamp-TOff();}
    Double_t TimeRaw() const {return fTimeStamp;}
    UShort_t Mod() const {return fModule;}
    UShort_t Chan() const {return fChannel;}
    UShort_t Charge() const {return fCharge;}
    UShort_t Type() const {return GetDetType(fModule,fChannel);}
    UShort_t Index()const {return GetIndex(fModule,fChannel);}
    Double_t TOff()const {return GetTOff(fModule,fChannel);}
    
};


class TelescopeHit{
    
    private :
    DetHit fdEHit,fEHit;
    UShort_t ab;
    
    public:
    
    TelescopeHit(){};
    TelescopeHit(const DetHit& dE,const DetHit& E):fdEHit(dE),fEHit(E){
        ab=AB(dE);
    };
    virtual ~TelescopeHit(){}; 
    
    DetHit& dE(){return fdEHit;}
    DetHit& E(){return fEHit;}
    
    Double_t Time(){return fdEHit.Time();}
    
    UShort_t AB(){return ab;};
    
    static int AB(const DetHit& hit);
    
    TVector3 GetPos(bool blur=false,bool intrinsic=false){
        return SiliconPosDet(fdEHit.Index(),fEHit.Index(),ab,blur,intrinsic);
    }
    
    
    static TVector3 SiliconDetectorPos(UShort_t N_dE,UShort_t N_E,bool Inv_dE=false,bool Inv_E=false,bool blur=false,bool intrinsic=false);

    static TVector3 SiliconPosDet(UShort_t N_dE,UShort_t N_E,int AB,bool blur=false,bool intrinsic=false);
    
    
    private :
    static double ActiveLength;
    static int segN;

    // Distance in the XZ plane between the face of detector and origin
    static double MagXZnorm_dE, MagXZnorm_E;

    // Distance between the origin facing PCB face and center/front of the silicon wafer
    static double FaceOffset_dE, FaceOffset_E;

    // Distance along the detector face between the face center and origin normal vector, along the XZ plane
    // As the face normal vector origin is NOT in the center
    static double FaceOriginOffsetXZ;

    // Calculate the basic parameters
    static double dStrip, XZ_dE, XZ_E, OffsetY, OffsetXZ;

    // The default assumption is the XZ strip 0 is nearest the beam axis and Y strip 0 is nearest floor
    static bool Invert_dEA, Invert_EA, Invert_dEB, Invert_EB;
    
    
    
};

#endif
