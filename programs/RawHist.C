#include <Digitisers.h>
#include <IO.h>

#include <TFile.h>
#include <TString.h>

#include <iostream>

int main(int argc, char** argv)
{
    if (argc < 2 || argc > 3) {
        std::cout << "Usage:\n"
                  << argv[0] << " bin_file_stem [-o|-O]\n";
        return 1;
    }

    const TString binStem = argv[1];
    bool overwrite = false;
    if (argc == 3) {
        const TString overwriteArg = argv[2];
        if (overwriteArg == "-o" || overwriteArg == "-O") {
            overwrite = true;
        } else {
            std::cout << "Usage:\n"
                      << argv[0] << " bin_file_stem [-o|-O]\n";
            return 1;
        }
    }
    TString outfilename = binStem;
    if (!outfilename.EndsWith(".root")) {
        outfilename += ".root";
    }

    std::vector<std::unique_ptr<DigitiserBase>> digitisers = BuildDigitiserList(binStem);
    if (digitisers.empty()) {
        std::cerr << "No digitiser .bin files found for stem " << binStem << '\n';
        return 2;
    }

    if (!TestOutputPath(outfilename, overwrite, "Histogram")) {
        return 3;
    }

    DigitiserAdcHistograms ADChists = BuildDigitiserAdcHistograms(digitisers);

    TFile outfile(outfilename, "RECREATE");
    if (outfile.IsZombie()) {
        std::cerr << "Could not create output file " << outfilename << '\n';
        return 4;
    }
    ADChists.SetDirectory(&outfile);

    Event ev;
    Long64_t totalEvents = 0;
    for (auto& digiPtr : digitisers) {
        if (!digiPtr) {
            continue;
        }

        auto& digi = *digiPtr;
        while (digi.getNextEvent(ev)) {
            ADChists.Fill(ev.mod, ev.ch, ev.adc);
            ++totalEvents;
        }
    }

    ADChists.Write();
    outfile.Write();
    outfile.Close();

    std::cout << "Processed " << totalEvents << " raw events\n";
    std::cout << "Wrote histograms to " << outfilename << '\n';
    return 0;
}
