#include <IO.h>

#include <iostream>

int main(int argc, char** argv)
{
    JAEASortIO io(argc, argv);

    std::cout << "ShowManual: " << io.ShowManual << "\n";
    std::cout << "BinInputStem: " << io.BinInputStem << "\n";
    std::cout << "EventTreeOutFilename: " << io.EventTreeOutFilename << "\n";
    std::cout << "HistogramOutFilename: " << io.HistogramOutFilename << "\n";
    std::cout << "EventInputFiles:\n";
    for (const auto& fileName : io.EventInputFiles) {
        std::cout << "  " << fileName << "\n";
    }

    return 0;
}
