#include <Detectors.h>
#include <IO.h>

#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

std::string StripComment(const std::string& line)
{
    const std::string::size_type pos = line.find('#');
    return pos == std::string::npos ? line : line.substr(0, pos);
}

template <typename T>
T ReadValue(std::istringstream& input, const std::string& name)
{
    T value;
    if (!(input >> value)) {
        throw std::runtime_error("missing or invalid " + name);
    }
    return value;
}

void RequireLineEnd(std::istringstream& input)
{
    std::string extra;
    if (input >> extra) {
        throw std::runtime_error("unexpected extra token: " + extra);
    }
}

void ForChannelRange(int first, int last, const std::function<void(int)>& action)
{
    const int direction = first <= last ? 1 : -1;
    for (int chan = first; chan != last + direction; chan += direction) {
        action(chan);
    }
}

void PrintCommandSyntax()
{
    std::cout
        << "Command file syntax:\n"
        << "  # comments are ignored\n"
        << "  output filename.cal\n"
        << "  type  mod chan DetectorType\n"
        << "  index mod chan index\n"
        << "  cal   mod chan p0 p1 p2\n"
        << "  toff  mod chan timeOffset\n"
        << "  range_type  mod firstChan lastChan DetectorType\n"
        << "  range_index mod firstChan lastChan firstIndex [indexStep]\n"
        << "  range_cal   mod firstChan lastChan p0 p1 p2\n"
        << "  range_toff  mod firstChan lastChan timeOffset\n\n"
        << "DetectorType must be one of:\n"
        << "  HPGe LaBr SiDeltaE Si SiDeltaE_B Si_B Solar Dice CdTe S3Ring S3Sector\n";
}

void PrintUsage(const char* program)
{
    std::cout
        << "Usage:\n"
        << "  " << program << " commands.txt [output.cal]\n\n";
    PrintCommandSyntax();
}

void ProcessCommand(const std::string& line, std::string& outputFile)
{
    std::istringstream input(line);
    std::string command;
    input >> command;

    if (command.empty()) {
        return;
    }

    if (command == "output") {
        outputFile = ReadValue<std::string>(input, "output filename");
        RequireLineEnd(input);
    } else if (command == "type" || command == "dettype") {
        const int mod = ReadValue<int>(input, "module");
        const int chan = ReadValue<int>(input, "channel");
        const std::string type = ReadValue<std::string>(input, "detector type");
        RequireLineEnd(input);
        DetHit::SetDetType(mod, chan, type);
    } else if (command == "index") {
        const int mod = ReadValue<int>(input, "module");
        const int chan = ReadValue<int>(input, "channel");
        const int index = ReadValue<int>(input, "index");
        RequireLineEnd(input);
        DetHit::SetIndex(mod, chan, index);
    } else if (command == "cal") {
        const int mod = ReadValue<int>(input, "module");
        const int chan = ReadValue<int>(input, "channel");
        const double p0 = ReadValue<double>(input, "p0");
        const double p1 = ReadValue<double>(input, "p1");
        const double p2 = ReadValue<double>(input, "p2");
        RequireLineEnd(input);
        DetHit::SetCalibrationParam(mod, chan, p0, p1, p2);
    } else if (command == "toff") {
        const int mod = ReadValue<int>(input, "module");
        const int chan = ReadValue<int>(input, "channel");
        const double offset = ReadValue<double>(input, "time offset");
        RequireLineEnd(input);
        DetHit::SetTOff(mod, chan, offset);
    } else if (command == "range_type") {
        const int mod = ReadValue<int>(input, "module");
        const int first = ReadValue<int>(input, "first channel");
        const int last = ReadValue<int>(input, "last channel");
        const std::string type = ReadValue<std::string>(input, "detector type");
        RequireLineEnd(input);
        ForChannelRange(first, last, [&](int chan) { DetHit::SetDetType(mod, chan, type); });
    } else if (command == "range_index") {
        const int mod = ReadValue<int>(input, "module");
        const int first = ReadValue<int>(input, "first channel");
        const int last = ReadValue<int>(input, "last channel");
        const int firstIndex = ReadValue<int>(input, "first index");
        int step = 1;
        if (!(input >> step)) {
            input.clear();
        }
        RequireLineEnd(input);
        int index = firstIndex;
        ForChannelRange(first, last, [&](int chan) {
            DetHit::SetIndex(mod, chan, index);
            index += step;
        });
    } else if (command == "range_cal") {
        const int mod = ReadValue<int>(input, "module");
        const int first = ReadValue<int>(input, "first channel");
        const int last = ReadValue<int>(input, "last channel");
        const double p0 = ReadValue<double>(input, "p0");
        const double p1 = ReadValue<double>(input, "p1");
        const double p2 = ReadValue<double>(input, "p2");
        RequireLineEnd(input);
        ForChannelRange(first, last, [&](int chan) { DetHit::SetCalibrationParam(mod, chan, p0, p1, p2); });
    } else if (command == "range_toff") {
        const int mod = ReadValue<int>(input, "module");
        const int first = ReadValue<int>(input, "first channel");
        const int last = ReadValue<int>(input, "last channel");
        const double offset = ReadValue<double>(input, "time offset");
        RequireLineEnd(input);
        ForChannelRange(first, last, [&](int chan) { DetHit::SetTOff(mod, chan, offset); });
    } else {
        throw std::runtime_error("unknown command: " + command);
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2 || std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
        PrintUsage(argv[0]);
        return argc < 2 ? 1 : 0;
    }

    const std::string inputFile = argv[1];
    std::string outputFile = argc > 2 ? argv[2] : "cal.txt";

    PrintCommandSyntax();
    std::cout << '\n';

    std::ifstream input(inputFile);
    if (!input.is_open()) {
        std::cerr << "Failed to open command file " << inputFile << '\n';
        return 1;
    }

    std::string line;
    int lineNumber = 0;
    try {
        while (std::getline(input, line)) {
            ++lineNumber;
            ProcessCommand(StripComment(line), outputFile);
        }
        if (argc > 2) {
            outputFile = argv[2];
        }
        WriteCal(outputFile);
    } catch (const std::exception& e) {
        std::cerr << inputFile << ':' << lineNumber << ": " << e.what() << '\n';
        return 2;
    }

    std::cout << "Wrote calibration file " << outputFile << '\n';
    return 0;
}
