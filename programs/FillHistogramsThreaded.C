#include <FillHistograms.h>

#include <TFile.h>
#include <TStopwatch.h>
#include <TString.h>
#include <TTree.h>

#include <iostream>
#include <stdexcept>

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cout << "Usage:\n"
                  << argv[0] << " input_events.root nworkers\n";
        return 1;
    }

    TString infilename = argv[1];
    unsigned int nworkers = std::stoi(argv[2]);

    TString outfilename = infilename;
    if (outfilename.EndsWith(".root")) {
        outfilename.ReplaceAll(".root", "_hist.root");
    } else {
        outfilename += "_hist.root";
    }

    try {
        TStopwatch timer;
        timer.Start();

        TFile* infile = TFile::Open(infilename, "READ");
        if (!infile || infile->IsZombie()) {
            throw std::runtime_error("Could not open input file");
        }

        TTree* intree = dynamic_cast<TTree*>(infile->Get("EventTree"));
        if (!intree) {
            infile->Close();
            throw std::runtime_error("Could not find TTree 'EventTree' in input file");
        }

        TFile* outfile = TFile::Open(outfilename, "RECREATE");
        if (!outfile || outfile->IsZombie()) {
            infile->Close();
            throw std::runtime_error("Could not create output file");
        }

        ThreadedHistogramSet histograms;
        FillHistogramsFromEventTree(intree, histograms, nworkers);

        outfile->cd();
        histograms.WriteAll(outfile);
        outfile->Write();
        outfile->Close();
        infile->Close();

        std::cout << "\nDone\n";

        timer.Stop();
        Double_t rtime = timer.RealTime();
        Double_t ctime = timer.CpuTime();
        std::cout << Form("\n RealTime = %d seconds, CpuTime = %d seconds\n\n",(Int_t)rtime,(Int_t)ctime );

        std::cout << "Wrote histograms to " << outfilename << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 2;
    }

    return 0;
}
