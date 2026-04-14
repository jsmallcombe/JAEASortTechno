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

using namespace std;

std::vector<TString> GetTreeSplitFileList(TString base);

class JAEASortIO{
    
    public:
    static void WriteCalibration(string filename="cal.txt");
    static void ReadCalibration(string filename="cal.txt");
    static void PrintManual(std::ostream& os = std::cout);
    
    private:
	vector<string> store;
    vector<double> NumericInputs;
    vector<TString> NumericInputNames;
    vector<TString> InputRootSpecs;

    void AddInputRootSpec(const TString& inputSpec);
    void ResolveInputFiles();
	
    public:
    TString BinInputStem;
    vector<TString> EventInputFiles;
    TString EventTreeOutFilename;
    TString HistogramOutFilename;
    vector<long> Entries;
    vector<TCutG*> CutGates;
    vector<UShort_t> GateID;
    bool ShowManual = false;
	

	stringstream infostream;
	void Rewind();
	
	JAEASortIO(){};
	JAEASortIO(int argc, char *argv[]);	
	virtual ~JAEASortIO(){
		for(auto g : CutGates)delete g;
	};
	
	JAEASortIO( const JAEASortIO &obj);
	JAEASortIO& operator=(const JAEASortIO& obj);
	
	void ReadInfoFile(string filename);
	void ProcessInputs();
	void ProcessOption(TString str);

	string ReturnFind(string compare) const;
	bool IsPresent(string compare) const;
	double Next(string compare) const;
	string NextString(string compare) const;
	void Next(string compare,double &ret) const {ret=Next(compare);}
	void NextTwo(string compare,double& ret,double& retB) const;
    
    bool TestInput(TString InputName) const;
    double GetInput(TString InputName) const;
	
    TChain* DataTree(TString TreeName);

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
	
#endif
