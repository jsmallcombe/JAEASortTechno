// // g++ SinglesBuild.C `root-config --cflags --libs` -o SingleSor
	
#include <Bin2RootClassy.h>
#include <MakeEventTreeSingle.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << "Usage:\n"
        << argv[0] << " input.root [output.root]  [tdiff] \n";
        return 1;
    }
    
    TString infilename  = argv[1];
    TString outfilename = (argc > 2) ? argv[2] : "";
    Long64_t tdiff      = (argc > 3) ? std::stoll(argv[3]) : 2000;
    
    try {
        Bin2RootClassy(infilename,"tmp.root",-1);
        MakeEventTreeNew("tmp.root", outfilename, false, tdiff);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
    
    return 0;
}
