	
#include <MergedThread.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage:\n"
        << argv[0] << " input.root [output.root] [chainmode] [tdiff] [BufferSize]\n";
        return 1;
    }
    
    TString infilename  = argv[1];
    TString outfilename = (argc > 2) ? argv[2] : "";
    bool chainmode      = (argc > 3) ? std::stoi(argv[3]) : false;
    Long64_t tdiff      = (argc > 4) ? std::stoll(argv[4]) : 2000;
    int BufferSize      = (argc > 5) ? std::stoi(argv[5]) : -1;
    
    try {
        //         MakeEventTreeFromBin(infilename, outfilename, chainmode, tdiff, BufferSize);
        MakeEventTreeFromBin(infilename, outfilename);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
    
    return 0;
}

