#include <IO.h>
#include <Detectors.h>

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

// =========================
// File list helper
// =========================
std::vector<TString> GetFileList(TString base)
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
        std::cerr << std::endl <<"Failed to open calibration file "<<filename<<" for writing." << std::flush;
        return;
    }

    outputFile<<"# Module / Channel / p0 / p1 / p2 / DetectorType / Index / TimeOffset"<< '\n';
    
    for (size_t i = 0; i < DetHit::ChanCal.size(); ++i) {
        for (size_t j = 0; j < DetHit::ChanCal[i].size(); ++j) {
            
            const auto& cal = DetHit::ChanCal[i][j];
            outputFile << i << ' ' << j << ' ' << cal.p0 << ' ' << cal.p1 << ' ' << cal.p2 << ' '
                       << cal.DetectorType << ' ' << cal.Index << ' ' << cal.TOff << '\n';
        }
    }
    
    outputFile.close();
}


void JAEASortIO::ReadCalibration(string filename){
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << std::endl << "Failed to open calibration file "<<filename<<" for reading." << std::flush;
        return;
    }
  
    DetHit::ChanCal.clear(); // Clear the existing data

    std::string line;
    while (std::getline(inputFile, line)) {
        std::istringstream iss(line);
        
        if(line[0]=='#')continue;
        
        int i, j;
        ChannelCalibration cal;
        if (iss >> i >> j >> cal.p0 >> cal.p1 >> cal.p2 >> cal.DetectorType >> cal.Index >> cal.TOff) {
            
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
            std::cerr << std::endl << "Error parsing line: " << line << std::flush;
        }
    }

    inputFile.close();
    if (inputFile.eof()) {
        std::cout << std::endl<< "Successfully read from the file: " << filename << std::flush;
    } else {
        std::cerr << std::endl<< "An error occurred while reading from the file: " << filename << std::flush;
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
		TString Tinpstr(argv[i]);
        // If argument is an info file process it as if it was command line input
		if(Tinpstr.EndsWith(".info")){
            
            ReadInfoFile(inpstr);
		}else{
			store.push_back(inpstr);
		}
	}
	
	Rewind();
    ProcessInputs();
};


void JAEASortIO::ReadInfoFile(string filename){
    ifstream infofile(filename);
    if(infofile.good()){
        cout<<endl<<"InfoFile : "<<filename<<" "<<flush;
        string fileline;
        
        // Doing line by line so that comment lines can be skipped
        while(getline(infofile, fileline, '\n')){
            if(!(fileline[0]=='#')){ //skip comments
                
                // Because we are reading line by line have to cast back up to a stream
                stringstream ss;
                ss<<fileline;
                string sep;
                while(ss>>sep){
                    //info files can be recursive
                    if(sep.find(".info")==sep.size()-5&&sep.size()>6){
                        ReadInfoFile(fileline);
                    }else{
                        store.push_back(sep);
                    }
                }
            }
        }
        infofile.close();
    }else{
        cout<<endl<<"Invalid infofile file specified : "<<filename<<flush;
    }	
}

void JAEASortIO::ProcessInputs(){
    vector<string> inputpattern;
    vector<string> runs;
    string outpattern="";
    OutFilename="Out.root";
    
    string str;
    while(*this>>str){
        
        // If a cal file, read it in to DetHit class
        if(str.find(".cal")==str.size()-4&&str.size()>5){
            ReadCalibration(str);
        }else if(str.find(".root")==str.size()-5&&str.size()>6){ // If a root file name
            
            if(str.find("**")<str.size()){
                // Double wild card to give input pattern
                inputpattern.push_back(str);
            }else if(str.find("*")<str.size()){
                // Single wild card to give output pattern
                outpattern=str;
            }else{
                //No wild card, direct file specification
                
                if(std::filesystem::exists(str)){//Exists, must be an input
                    TFile filetest(str.c_str());
                    if(filetest.IsOpen()){
                        filetest.Close();
                        inputpattern.push_back(str);
                    }
                }else{//Doesnt exist, must be the output
                    OutFilename=str;
                }
            }
        }else if(str.find("Mode")<str.size()){
            // Set the program mode i.e. sort or calibrat            
        }else if(str[0]=='-'){
            // A special argument, read the next item out of order of normal processing loop
            
            ProcessOption(str);
            
        }else{
            int run;
            if(stringToInt(str,run)){
//                 runs.push_back(run);
                runs.push_back(str);
            }else{
                cout<<endl<<"UNKNOWN COMMAND LINE INPUT  "<<str<<flush;
            }
        }
    }
    
    
    // Now we have processed all inputs, try to determine the list of input files and output file name
    
    if(outpattern.size()&&runs.size()){
        size_t pos = outpattern.find("*");
        string prefix=outpattern.substr(0, pos);
        string suffix=outpattern.substr(pos + 1);
        
        for(auto r : runs){
            prefix+=r+"_";
        }
        OutFilename=prefix.substr(0,prefix.size()-1)+suffix;
    }
    
    
    InputFiles.clear();
    for(auto str : inputpattern){
        
        //Because it could be a mix of exact files and patters, check again
        
        if(str.find("**")<str.size()){
            size_t pos = str.find("**");
            string prefix=str.substr(0, pos);
            string suffix=str.substr(pos + 2);
            
            // If runs specified use them, else use everything that matches
            if(runs.size()>0){

                for(auto r : runs){
                    string testpath=prefix+r+suffix;
                    
                    TFile filetest(testpath.c_str());
                    if(filetest.IsOpen()){//Exists
                        filetest.Close();
                        InputFiles.push_back(testpath);
                    }
                }                
                
            }else{
                
                string path="";
                if(!prefix.find("/")<prefix.size()){
                    path=prefix.substr(0,prefix.rfind("/"));
                    prefix=prefix.substr(prefix.rfind("/")+1);
                }
                
                void* dir = gSystem->OpenDirectory(path.c_str());
                if (!dir) {
                    std::cerr<< std::endl << "Cannot open directory: " << path<< std::flush; 
                    continue;
                }

                const char* entry;

                std::vector<TString> DirectoryItems;
                while ((entry = gSystem->GetDirEntry(dir))) {
                    DirectoryItems.emplace_back(entry);
                }
                std::sort(DirectoryItems.begin(), DirectoryItems.end());
                    
                for(auto fileName : DirectoryItems){
                    
                    if (fileName.BeginsWith(prefix.c_str()) && fileName.EndsWith(suffix.c_str())) {
                        TString fullPath = TString(path) + "/" + fileName;
                        TFile* file = TFile::Open(fullPath);
                        if (file && file->IsOpen()) {
                            file->Close();
                            InputFiles.push_back(fullPath);
                        }
                    }
                }

                gSystem->FreeDirectory(dir);
            }
                
        }else{
            InputFiles.push_back(str);
           // str is an exact file, the existence of which was already checked
        }
    }
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
 
void JAEASortIO::ProcessOption(TString str){
        // A special argument, read the next item out of order of normal processing loop

// Contains 
        if(str.EqualTo("-o")||str.EqualTo("-O")){// Overwrite an existing output file, next argument is file name
            *this>>str;
            if(str.EndsWith(".root")){ // If a root file name
                OutFilename=str;
            }
        }else if(str.EqualTo("-ID")){// Load a particle ID gate, next argument file containing name
            *this>>str;
            if(str.EndsWith(".root")){ // If a root file name
                
                    
                TFile *file = TFile::Open(str, "READ");
                if (!file || file->IsZombie()) {
                    std::cerr<< std::endl << "Error: Could not open file " << str <<std::flush;
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
                            std::cout<< std::endl << "Found a TCutG object: " << cutG->GetName() << std::flush;
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
        
            std::cout<< std::endl << str<<" : " << inputdata << std::flush;
        }
}
    
    

void JAEASortIO::Rewind(){
	infostream.str("");
	infostream.clear(); // Clear state flags.
	for(int i=0;i<store.size();i++)infostream<<store[i]<<" ";
	
}

string JAEASortIO::ReturnFind(string compare){
	for(int i=0;i<store.size();i++){string str=store[i];
		if(str.find(compare)<str.size())return str;
	}	
	return "";
}

bool JAEASortIO::IsPresent(string compare){
	return ReturnFind(compare).size();
}

double JAEASortIO::Next(string compare){
	for(int i=0;i<store.size()-1;i++){string str=store[i];
		if(str.find(compare)<str.size()){
			stringstream ss;ss<<store[i+1];
			double ret;ss>>ret;
			return ret;
		}
	}
	return 0;
}

string JAEASortIO::NextString(string compare){
	for(int i=0;i<store.size()-1;i++){string str=store[i];
		if(str.find(compare)<str.size()){
			return store[i+1];
		}
	}
	return "";
}


void JAEASortIO::NextTwo(string compare,double& ret,double& retB){
	for(int i=0;i<store.size()-2;i++){
		string str=store[i];
		if(str.find(compare)<str.size()){
			stringstream ss;
			ss<<store[i+1]<<" "<<store[i+2];
			ss>>ret>>retB;
			return;
		}
	}
}

TChain* JAEASortIO::DataTree(TString TreeName){
    Entries.clear();
    
	TChain* DataChain = new TChain(TreeName,TreeName);
    for(auto FileName : InputFiles){
        
        TFile filetest(FileName); // Shouldnt be needed as we already tests 
        if(filetest.IsOpen()){
            cout<<endl<<"Added data : "<<FileName<<flush;
            filetest.Close();
            DataChain->Add(FileName);
            Entries.push_back(DataChain->GetEntries());
        }else{
            cout<<endl<<"Invalid data file : "<<FileName<<flush;
        }
	}
	
	return DataChain;
}


bool JAEASortIO::TestInput(TString InputName){
    for(auto& s : NumericInputNames){
        if(s==InputName)return true;
    }
    return false;
}
double JAEASortIO::GetInput(TString InputName){
    for(unsigned int i=0;i<NumericInputNames.size();i++){
        if(NumericInputNames[i]==InputName)return NumericInputs[i];
    }
    return 0;
}
