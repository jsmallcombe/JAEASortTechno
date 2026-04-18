#ifndef JAEASortIOHead
#define JAEASortIOHead

#include <TROOT.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <TTree.h>
#include <TFile.h>
#include <TChain.h>
#include <TSystem.h>
#include <TString.h>
#include <TKey.h>
#include <TCutG.h>
#include <filesystem>
#include <ostream>
#include <Digitisers.h>

using namespace std;

class JAEASortIO;
extern JAEASortIO* gIO;

std::vector<TString> GetTreeSplitFileList(TString base);

class JAEASortIO{
    
    public:
    
	JAEASortIO(){};
	JAEASortIO(int argc, char *argv[]);	
	virtual ~JAEASortIO(){
		for(auto g : CutGates)delete g;
	};
	JAEASortIO(const JAEASortIO&) = delete;
	JAEASortIO& operator=(const JAEASortIO&) = delete;

    static void WriteCalibration(string filename="cal.txt");
    static void ReadCalibration(string filename="cal.txt");
    static void PrintManual(std::ostream& os = std::cout);

	stringstream infostream;

    vector<long> Entries;
    vector<TString> EventInputFiles;

    TString BinInputStem;
    vector<std::unique_ptr<DigitiserBase>> Digitisers;
    TString EventTreeOutFilename;
    TString TreeOutputPath;
    TString HistogramOutFilename;
    
    vector<TCutG*> CutGates;
    vector<UShort_t> GateID;

    bool WriteEventTree = false;
    bool DoHistSort = false;
    bool Overwrite = false;
    bool Validated;

	// string ReturnFind(string compare) const;
	// bool IsPresent(string compare) const;
	// double Next(string compare) const;
	// string NextString(string compare) const;
	// void Next(string compare,double &ret) const {ret=Next(compare);}
	// void NextTwo(string compare,double& ret,double& retB) const;
    
    bool TestInput(TString InputName) const;
    double GetInput(TString InputName,double=0) const;
	
    TChain* DataTree(TString TreeName);
    
    private:
	vector<string> store;
    vector<double> NumericInputs;
    vector<TString> NumericInputNames;
    vector<TString> InputRootSpecs;

    void AddInputRootSpec(const TString& inputSpec);
	bool ProcessInputs();
    void ResolveInputFiles();
    bool ValidateFiles();
	void ReadInfoFile(string filename);
	void ProcessOption(TString str);
	void Rewind();


};

//I'm still suprised this worked
// template <typename T>
// JAEASortIO& operator>>(JAEASortIO& is, T& obj){
// 	is.infostream>>obj;
// 	return is;
// }

//This new doesnt work something something
template <typename T>
stringstream& operator>>(JAEASortIO& is, T& obj){
	is.infostream>>obj;
	return is.infostream;
}

template <typename T>
stringstream& operator<<(JAEASortIO& is, T& obj){
	is.infostream<<obj;
	return is.infostream;
}


void WriteCal(string filename="cal.txt");
void ReadCal(string filename="cal.txt");

bool stringToInt(const std::string& str, int& result);

TString StripFileName(TString str);
bool TestOutputPath(const TString& filename, bool overwrite = false, const char* label = "Output");
	
#endif
