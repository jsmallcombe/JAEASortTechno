#include <IO.h>
#include <Detectors.h>

#include <algorithm>
#include <regex>
#include <unordered_set>

#include "EmbeddedManual.h"

JAEASortIO* gIO = nullptr;

namespace {

bool EndsWith(const std::string& value, const std::string& suffix)
{
    return value.size() >= suffix.size()
        && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool IsRootPath(const std::string& value)
{
    return EndsWith(value, ".root");
}

bool IsImplicitRootWildcard(const std::string& value)
{
    return !value.empty() && value.back() == '*' && !IsRootPath(value);
}

bool IsInfoPath(const std::string& value)
{
    return EndsWith(value, ".info");
}

bool IsCalibrationPath(const std::string& value)
{
    return EndsWith(value, ".cal");
}

bool HasWildcard(const std::string& value)
{
    return value.find('*') != std::string::npos;
}

std::string NormalizePath(const std::filesystem::path& path)
{
    try {
        return std::filesystem::weakly_canonical(path).string();
    } catch (...) {
        return std::filesystem::absolute(path).lexically_normal().string();
    }
}

bool IsSplitRootFile(const std::filesystem::path& path)
{
    static const std::regex splitPattern(".*_[0-9]+$");
    return std::regex_match(path.stem().string(), splitPattern);
}

std::string WildcardToRegex(const std::string& pattern)
{
    std::string regexPattern;
    regexPattern.reserve(pattern.size() * 2);
    regexPattern += '^';

    for (char c : pattern) {
        switch (c) {
            case '*':
                regexPattern += ".*";
                break;
            case '.':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
            case '+':
            case '?':
            case '^':
            case '$':
            case '|':
            case '\\':
                regexPattern += '\\';
                regexPattern += c;
                break;
            default:
                regexPattern += c;
                break;
        }
    }

    regexPattern += '$';
    return regexPattern;
}

std::vector<TString> ExpandSplitRootFamily(const std::filesystem::path& inputPath)
{
    std::vector<TString> files;

    if (!std::filesystem::exists(inputPath) || !IsRootPath(inputPath.string())) {
        return files;
    }

    files.push_back(TString(NormalizePath(inputPath)));

    if (IsSplitRootFile(inputPath)) {
        return files;
    }

    const std::filesystem::path parent = inputPath.parent_path();
    const std::string stem = inputPath.stem().string();

    for (int i = 1;; ++i) {
        const std::filesystem::path splitPath = parent / (stem + "_" + std::to_string(i) + ".root");
        if (!std::filesystem::exists(splitPath)) {
            break;
        }
        files.push_back(TString(NormalizePath(splitPath)));
    }

    return files;
}

std::vector<TString> ResolveRootInputSpec(const TString& inputSpec)
{
    std::vector<TString> files;
    const std::string spec = inputSpec.Data();

    if (!IsRootPath(spec)) {
        return files;
    }

    if (!HasWildcard(spec)) {
        return ExpandSplitRootFamily(std::filesystem::path(spec));
    }

    const std::filesystem::path specPath(spec);
    std::filesystem::path directory = specPath.parent_path();
    if (directory.empty()) {
        directory = ".";
    }

    if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
        std::cerr << "Cannot open directory: " << directory.string() << std::endl;
        return files;
    }

    const std::regex pattern(WildcardToRegex(specPath.filename().string()));
    std::vector<std::filesystem::path> matches;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::filesystem::path filePath = entry.path();
        if (!IsRootPath(filePath.string())) {
            continue;
        }

        if (std::regex_match(filePath.filename().string(), pattern)) {
            matches.push_back(filePath);
        }
    }

    std::sort(matches.begin(), matches.end());

    for (const auto& match : matches) {
        const std::vector<TString> expanded = ExpandSplitRootFamily(match);
        files.insert(files.end(), expanded.begin(), expanded.end());
    }

    return files;
}

void CloneCutGates(const std::vector<TCutG*>& source, std::vector<TCutG*>& target)
{
    target.clear();
    target.reserve(source.size());
    for (TCutG* gate : source) {
        target.push_back(gate ? static_cast<TCutG*>(gate->Clone()) : nullptr);
    }
}

}

// =========================
// File list helper
// =========================
std::vector<TString> GetTreeSplitFileList(TString base)
{
    std::vector<TString> files;
    
    files.push_back(base);
    
    TString name = base;
    name.ReplaceAll(".root","");
    
    for(int i=1;;i++){
        TString fname;
        fname.Form("%s_%d.root", name.Data(), i);
        
        if (gSystem->AccessPathName(fname)) break;
        
        files.push_back(fname);
    }
    
    return files;
}

void JAEASortIO::WriteCalibration(string filename){
    std::ofstream outputFile(filename);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open calibration file "<<filename<<" for writing." << std::endl;
        return;
    }

    outputFile<<"# Module / Channel / p0 / p1 / p2 / DetectorType / Index / TimeOffset"<< '\n';
    
    for (size_t i = 0; i < DetHit::ChanCal.size(); ++i) {
        for (size_t j = 0; j < DetHit::ChanCal[i].size(); ++j) {
            
            const auto& cal = DetHit::ChanCal[i][j];
            outputFile << i << ' ' << j << ' ' << cal.p0 << ' ' << cal.p1 << ' ' << cal.p2 << ' '
                       << DetHit::DetTypeName(cal.DetectorType) << ' ' << cal.Index << ' ' << cal.TOff << '\n';
        }
    }
    
    outputFile.close();
}


void JAEASortIO::ReadCalibration(string filename){
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open calibration file "<<filename<<" for reading." << std::endl;
        return;
    }
  
    DetHit::ChanCal.clear(); // Clear the existing data

    std::string line;
    while (std::getline(inputFile, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream iss(line);
        
        if(line[0]=='#')continue;
        
        int i, j;
        ChannelCalibration cal;
        std::string detTypeToken;
        if (iss >> i >> j >> cal.p0 >> cal.p1 >> cal.p2 >> detTypeToken >> cal.Index >> cal.TOff) {
            try {
                int detTypeValue = 0;
                if (stringToInt(detTypeToken, detTypeValue)) {
                    cal.DetectorType = detTypeValue;
                } else {
                    cal.DetectorType = DetHit::ParseDetType(detTypeToken);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing detector type in line: " << line << " (" << e.what() << ")" << std::endl;
                continue;
            }
            
             // Resize the outer vector if necessary
            if (i >= static_cast<int>(DetHit::ChanCal.size())) {
                DetHit::ChanCal.resize(i + 1);
            }
            // Resize the inner vector if necessary
            if (j >= static_cast<int>(DetHit::ChanCal[i].size())) {
                DetHit::ChanCal[i].resize(j + 1);
            }
            // Set the calibration in the appropriate location
            DetHit::ChanCal[i][j] = cal;
        } else {
            std::cerr << "Error parsing line: " << line << std::endl;
        }
    }

    inputFile.close();
    if (inputFile.eof()) {
        std::cout << "Successfully read from the file: " << filename << std::endl;
    } else {
        std::cerr << "An error occurred while reading from the file: " << filename << std::endl;
    }
}



void WriteCal(string filename){JAEASortIO::WriteCalibration(filename);};
void ReadCal(string filename){JAEASortIO::ReadCalibration(filename);};

bool stringToInt(const std::string& str, int& result) {
    try {
        size_t pos;
        result = std::stoi(str, &pos);
        if (pos < str.length()) {
            // If there are any characters left suffix the number, it's not a valid integer
            return false;
        }
        return true;
    } catch (const std::invalid_argument& e) {
        // std::invalid_argument is thrown if the input is not a valid integer
        return false;
    } catch (const std::out_of_range& e) {
        // std::out_of_range is thrown if the input is out of the range of representable values for an int
        return false;
    }
}

////////////////////////////////////////


JAEASortIO::JAEASortIO(int argc, char *argv[]):JAEASortIO(){
    
    store.clear();
    
    // Itterate over command line inputs, skipping argument 0 which is the program
	for(int i=1;i<argc;i++){
		string inpstr(argv[i]);
        // If argument is an info file process it as if it was command line input
		if(IsInfoPath(inpstr)){
            
            ReadInfoFile(inpstr);
		}else{
			store.push_back(inpstr);
		}
	}
	
	Rewind();
    Validated=ProcessInputs();
    std::cout<<std::endl;
};


void JAEASortIO::ReadInfoFile(string filename){
    ifstream infofile(filename);
    if(infofile.good()){
        std::cout<<"InfoFile : "<<filename<<" "<<std::endl;
        string fileline;
        
        // Doing line by line so that comment lines can be skipped
        while(getline(infofile, fileline, '\n')){
            if (fileline.empty()) {
                continue;
            }
            if(!(fileline[0]=='#')){ //skip comments
                
                // Because we are reading line by line have to cast back up to a stream
                stringstream ss;
                ss<<fileline;
                string sep;
                while(ss>>sep){
                    //info files can be recursive
                    if(IsInfoPath(sep)){
                        ReadInfoFile(sep);
                    }else{
                        store.push_back(sep);
                    }
                }
            }
        }
        infofile.close();
    }else{
        std::cout<<"Invalid infofile file specified : "<<filename<<std::endl;
    }	
}

#include <cstdlib>
void JAEASortIO::PrintManual(std::ostream& os)
{
    os.write(reinterpret_cast<const char*>(README), README_len);
    std::exit(0);
}

void JAEASortIO::AddInputRootSpec(const TString& inputSpec)
{
    std::string spec = inputSpec.Data();
    if (IsImplicitRootWildcard(spec)) {
        spec += ".root";
    }

    if (IsRootPath(spec)) {
        InputRootSpecs.push_back(TString(spec));
    }
}

void JAEASortIO::ResolveInputFiles()
{
    EventInputFiles.clear();

    std::unordered_set<std::string> seenFiles;
    for (const auto& inputSpec : InputRootSpecs) {
        const std::vector<TString> resolved = ResolveRootInputSpec(inputSpec);
        for (const auto& fileName : resolved) {
            const std::string normalized = fileName.Data();
            if (seenFiles.insert(normalized).second) {
                EventInputFiles.push_back(fileName);
            }
        }
    }

}

bool JAEASortIO::ProcessInputs(){
    InputRootSpecs.clear();
    EventInputFiles.clear();
    BinInputStem = "";
    EventTreeOutFilename = "";
    TreeOutputPath = "";
    HistogramOutFilename = "";
    WriteEventTree = false;
    Overwrite = false;
    
    string str;
    while(*this>>str){
        
        // If a cal file, read it in to DetHit class
        if(IsCalibrationPath(str)){
            ReadCalibration(str);
        }else if(IsRootPath(str) || IsImplicitRootWildcard(str)){ // If a root file name
            if(HasWildcard(str) || std::filesystem::exists(str)){
                AddInputRootSpec(str);
            }else{
                std::cout<<"UNKNOWN ROOT INPUT "<<str<<" (use -hist file.root for outputs)"<<std::endl;
            }
        }else if(str[0]=='-'){
            // A special argument, read the next item out of order of normal processing loop
            ProcessOption(str);
            
        }else{
            if(BinInputStem.Length()==0){
                BinInputStem = str;
            }else{
                std::cout<<"UNKNOWN COMMAND LINE INPUT  "<<str<<std::endl;
            }
        }
    }
    std::cout<< std::endl;
    return ValidateFiles();

}

bool JAEASortIO::ValidateFiles(){
    ResolveInputFiles();

    if(BinInputStem.Length() > 0){
        if (EventInputFiles.size() > 0){
            std::cout<<"PROVIDED .bin AND .root INPUTS. LIMIT TO EITHER RAW .bin OR EVENT .root INPUT."<<std::endl;
            return false;
        }

        Digitisers=BuildDigitiserList(BinInputStem);
        if(!Digitisers.size()){
            std::cout<<"FAILED TO LOCATE SPECIFIED .bin FILES "<<BinInputStem<<std::endl;
            return false;
        }else{
            std::cout<<"Raw run .bin file stem used "<<BinInputStem<<std::endl;

            if(DoHistSort&&HistogramOutFilename.Length() == 0) {
                HistogramOutFilename = BinInputStem+"_hist.root";                
            }
        }

        // Build the output tree string even if it isnt going to be used
        std::filesystem::path outputStem(BinInputStem.Data());
        std::filesystem::path outputFile = outputStem;
        outputFile += ".root";

        if (TreeOutputPath.Length() > 0) {
            const std::filesystem::path treeDirectory(TreeOutputPath.Data());
            outputFile = treeDirectory / outputFile.filename();
        }

        EventTreeOutFilename = outputFile.string();

    }

    if(WriteEventTree){
        if(BinInputStem.Length() > 0) {
            std::cout<<"TTree Output File "<<EventTreeOutFilename<<std::endl;
        }else{
            std::cout<<"TTREE OUTPUT REQUESTED BUT NO .bin FILES PROVIDED."<<std::endl;
            return false;
        }
    }

    if(WriteEventTree && EventInputFiles.size() > 0){
        std::cout<<"ERROR: REQUESTED TTREE BUILDING AND PROVIDED TTREE INPUT."<<std::endl;
        return false;
    }

    int fi=EventInputFiles.size();
    if (fi > 0 && HistogramOutFilename.Length() == 0) {
        DoHistSort=true;
        HistogramOutFilename = EventInputFiles[0];
        TString suffix="_hist.root";
        if(fi>1) suffix=TString::Format("x%d_hist.root", fi);
        HistogramOutFilename.Replace(HistogramOutFilename.Length() - 5, 5, suffix);
    }

    if (DoHistSort) {
        std::cout<<"Histogram Output File "<<HistogramOutFilename<<std::endl;
    }

    if(!(WriteEventTree||DoHistSort)){
        std::cout<<"NO OUTPUT MODE SELECTED. DEFAULTING TO TREE."<<std::endl;
        WriteEventTree = true;
        // return false;
    }

    return true;
}

TString StripFileName(TString str){
        TString fileName(str);

        fileName.Remove(fileName.Length() - 5, 5);

        // Find the last occurrence of the path separator
        if (fileName.Contains('/')) {
            fileName.Remove(0, fileName.Last('/') + 1);
        }

        return fileName;
}

bool TestOutputPath(const TString& filename, bool overwrite, const char* label)
{
    if (filename.Length() == 0) {
        std::cerr << "Missing " << label << " output filename\n";
        return false;
    }

    const std::filesystem::path outputPath(filename.Data());
    const std::filesystem::path parentPath = outputPath.parent_path();

    if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
        std::filesystem::create_directories(parentPath);
    }

    if (std::filesystem::exists(outputPath) && !overwrite) {
        std::cerr << label << " output already exists: " << filename
                  << " (use -O to overwrite)\n";
        return false;
    }

    return true;
}
 
void JAEASortIO::ProcessOption(TString str){
        str.ToLower();

        // A special argument, read the next item out of order of normal processing loop

// Contains 
        if(str.EqualTo("-man") || str.EqualTo("--man") || str.EqualTo("--help")){
            PrintManual();
        }else if(str.EqualTo("-bin")){
            *this>>str;
            BinInputStem = str;
        }else if(str.EqualTo("-tree")||str.EqualTo("-T")){
            WriteEventTree = true;
        }else if(str.EqualTo("-treepath")||str.EqualTo("-treedir")){
            *this>>str;
            TreeOutputPath = str;
            WriteEventTree = true;
        }else if(str.EqualTo("-h")){
            DoHistSort=true;
        }else if(str.EqualTo("-hist")){
            *this>>str;
            DoHistSort=true;
            if(IsRootPath(str.Data())){ // If a root file name
                HistogramOutFilename=str;
            }
        }else if(str.EqualTo("-o")){
            Overwrite = true;
        }else if(str.EqualTo("-apv8104")){
            *this>>APV8104::ModuleZeroIndex;
        }else if(str.EqualTo("-apv8032")){
            *this>>APV8032::ModuleZeroIndex;
        }else if(str.EqualTo("-apv8016a")){
            *this>>APV8016A::ModuleZeroIndex;
        }else if(str.EqualTo("-id")){// Load a particle ID gate, next argument file containing name
            *this>>str;
            if(IsRootPath(str.Data())){ // If a root file name
                
                    
                TFile *file = TFile::Open(str, "READ");
                if (!file || file->IsZombie()) {
                    std::cerr<< "Error: Could not open file " << str << std::endl;
                    return;
                }
                
                TString fileName=StripFileName(str);

                // Iterate over the keys in the file
                TIter nextKey(file->GetListOfKeys());
                TKey *key;
                while ((key = (TKey*)nextKey())) {
                    // Check if the class name matches "TCutG"
                    if (std::string(key->GetClassName()) == "TCutG") {
                        TCutG *cutG = (TCutG*)key->ReadObj();
                        if (cutG) {
                            gROOT->cd();
                            CutGates.push_back((TCutG*)cutG->Clone(fileName));
                            std::cout<< "Found a TCutG object: " << cutG->GetName() << std::endl;
                        }
                        break; // Exit the loop after finding the first TCutG
                    }
                }

                // Close the file
                file->Close();
                delete file;
                
                UShort_t GateTypeID;
                *this>>GateTypeID;
                GateID.push_back(GateTypeID);
            }
                   
        }else{
            str.Remove(TString::kLeading,'-');
            double inputdata;
            *this>>inputdata;
            
            NumericInputNames.push_back(str);
            NumericInputs.push_back(inputdata);
        
            std::cout<< str<<" : " << inputdata << std::endl;
        }
}
    
    

void JAEASortIO::Rewind(){
	infostream.str("");
	infostream.clear(); // Clear state flags.
	for(int i=0;i<store.size();i++)infostream<<store[i]<<" ";
	
}

// string JAEASortIO::ReturnFind(string compare) const{
// 	for(int i=0;i<store.size();i++){string str=store[i];
// 		if(str.find(compare)<str.size())return str;
// 	}	
// 	return "";
// }

// bool JAEASortIO::IsPresent(string compare) const{
// 	return ReturnFind(compare).size();
// }

// double JAEASortIO::Next(string compare) const{
// 	for(int i=0;i<store.size()-1;i++){string str=store[i];
// 		if(str.find(compare)<str.size()){
// 			stringstream ss;ss<<store[i+1];
// 			double ret;ss>>ret;
// 			return ret;
// 		}
// 	}
// 	return 0;
// }

// string JAEASortIO::NextString(string compare) const{
// 	for(int i=0;i<store.size()-1;i++){string str=store[i];
// 		if(str.find(compare)<str.size()){
// 			return store[i+1];
// 		}
// 	}
// 	return "";
// }

// void JAEASortIO::NextTwo(string compare,double& ret,double& retB) const{
// 	for(int i=0;i<store.size()-2;i++){
// 		string str=store[i];
// 		if(str.find(compare)<str.size()){
// 			stringstream ss;
// 			ss<<store[i+1]<<" "<<store[i+2];
// 			ss>>ret>>retB;
// 			return;
// 		}
// 	}
// }

TChain* JAEASortIO::DataTree(TString TreeName){
    Entries.clear();
    
	TChain* DataChain = new TChain(TreeName,TreeName);
    for(auto FileName : EventInputFiles){
        
        TFile filetest(FileName); // Shouldnt be needed as we already tests 
        if(filetest.IsOpen()){
            std::cout<<"Added data : "<<FileName<<std::endl;
            filetest.Close();
            DataChain->Add(FileName);
            Entries.push_back(DataChain->GetEntries());
        }else{
            std::cout<<"Invalid data file : "<<FileName<<std::endl;
        }
	}
	if(Entries.size()>0){
	    return DataChain;
    }else{
        delete DataChain;
        return nullptr;
    }
}

bool JAEASortIO::TestInput(TString InputName) const{
    InputName.ToLower();
    for(auto& s : NumericInputNames){
        if(s==InputName)return true;
    }
    return false;
}
double JAEASortIO::GetInput(TString InputName,double dflt) const{
    InputName.ToLower();
    for(unsigned int i=0;i<NumericInputNames.size();i++){
        if(NumericInputNames[i]==InputName)return NumericInputs[i];
    }
    return dflt;
}

bool JAEASortIO::GetBoolInput(TString InputName,bool dflt) const{
    return GetInput(InputName, dflt ? 1.0 : 0.0) != 0.0;
}
