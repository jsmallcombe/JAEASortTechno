#ifndef JAEASortHistogramRuntime
#define JAEASortHistogramRuntime

#include <atomic>
#include <chrono>
#include <iosfwd>

struct HistogramRuntimeOptions {
    bool basicHistogramsOnly = false;
    bool enableHistogramTimers = false;
};

struct HistogramRuntimeTimers {
    std::atomic<unsigned long long> fillBasicNs{0};
    std::atomic<unsigned long long> fillHistChunk1Ns{0};
    std::atomic<unsigned long long> fillHistChunk2Ns{0};
    std::atomic<unsigned long long> fillHistChunk3Ns{0};
    std::atomic<unsigned long long> fillHistChunk4Ns{0};
};

extern HistogramRuntimeOptions gHistogramRuntimeOptions;
extern HistogramRuntimeTimers gHistogramRuntimeTimers;

void ResetHistogramRuntimeTimers();
void PrintHistogramRuntimeTimers(std::ostream& os);

class HistogramScopedTimer {
public:
    explicit HistogramScopedTimer(std::atomic<unsigned long long>& total);
    ~HistogramScopedTimer();

private:
    std::atomic<unsigned long long>* total_ = nullptr;
    std::chrono::steady_clock::time_point start_;
};

class HistogramChunkTimer {
public:
    explicit HistogramChunkTimer(std::atomic<unsigned long long>& total);
    ~HistogramChunkTimer();

    void Next(std::atomic<unsigned long long>& total);

private:
    void Flush();

    std::atomic<unsigned long long>* current_ = nullptr;
    std::chrono::steady_clock::time_point start_;
};

#endif
