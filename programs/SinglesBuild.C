// // g++ SinglesBuild.C `root-config --cflags --libs` -o SingleSor
	
#include <Bin2RootClassy.h>
#include <MakeEventTreeSingle.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage:\n"
        << argv[0] << " input.root [output.root] [chunk] [chainmode] [tdiff] [BufferSize]\n";
        return 1;
    }
    
    TString infilename  = argv[1];
    TString outfilename = (argc > 2) ? argv[2] : "";
    Long64_t chunk      = (argc > 3) ? std::stoi(argv[3]) : 100;
    bool chainmode      = (argc > 4) ? std::stoi(argv[4]) : true;
    Long64_t tdiff      = (argc > 5) ? std::stoll(argv[5]) : 2000;
    int BufferSize      = (argc > 6) ? std::stoi(argv[6]) : -1;
    
    try {
        
        Bin2RootClassy(infilename,"tmp.root",chunk);
        MakeEventTreeNew("tmp.root", outfilename, chainmode, tdiff, BufferSize);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
    
    return 0;
}
