#include <ThreadedSort.h>

#include <TStopwatch.h>


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
        TStopwatch timer;
        timer.Start();

        //         MakeEventTreeFromBin(infilename, outfilename, chainmode, tdiff, BufferSize);
        MakeEventTreeFromBin(infilename, outfilename, tdiff, gBinChunkDefaultSize, BufferSize);

        timer.Stop();
        Double_t rtime = timer.RealTime();
        Double_t ctime = timer.CpuTime();
        std::cout << "\nDone\n";
        std::cout << Form("\n RealTime = %d seconds, CpuTime = %d seconds\n\n",
                          (Int_t)rtime,
                          (Int_t)ctime );
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }
    
    return 0;
}
