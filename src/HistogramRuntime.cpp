#include <HistogramRuntime.h>

#include <iomanip>
#include <iostream>

HistogramRuntimeOptions gHistogramRuntimeOptions;
HistogramRuntimeTimers gHistogramRuntimeTimers;

namespace {

double NsToSeconds(unsigned long long ns)
{
    return static_cast<double>(ns) / 1000000000.0;
}

}

void ResetHistogramRuntimeTimers()
{
    gHistogramRuntimeTimers.fillBasicNs.store(0, std::memory_order_relaxed);
    gHistogramRuntimeTimers.fillHistChunk1Ns.store(0, std::memory_order_relaxed);
    gHistogramRuntimeTimers.fillHistChunk2Ns.store(0, std::memory_order_relaxed);
    gHistogramRuntimeTimers.fillHistChunk3Ns.store(0, std::memory_order_relaxed);
    gHistogramRuntimeTimers.fillHistChunk4Ns.store(0, std::memory_order_relaxed);
}

void PrintHistogramRuntimeTimers(std::ostream& os)
{
    os << "\nHistogram timers\n";
    os << std::fixed << std::setprecision(3);
    os << "  FillHistogramsBasic total : "
       << NsToSeconds(gHistogramRuntimeTimers.fillBasicNs.load(std::memory_order_relaxed))
       << " s\n";
    os << "  FillHistograms chunk 1    : "
       << NsToSeconds(gHistogramRuntimeTimers.fillHistChunk1Ns.load(std::memory_order_relaxed))
       << " s\n";
    os << "  FillHistograms chunk 2    : "
       << NsToSeconds(gHistogramRuntimeTimers.fillHistChunk2Ns.load(std::memory_order_relaxed))
       << " s\n";
    os << "  FillHistograms chunk 3    : "
       << NsToSeconds(gHistogramRuntimeTimers.fillHistChunk3Ns.load(std::memory_order_relaxed))
       << " s\n";
    os << "  FillHistograms chunk 4    : "
       << NsToSeconds(gHistogramRuntimeTimers.fillHistChunk4Ns.load(std::memory_order_relaxed))
       << " s\n";
}

HistogramScopedTimer::HistogramScopedTimer(std::atomic<unsigned long long>& total)
{
    if (!gHistogramRuntimeOptions.enableHistogramTimers) {
        return;
    }
    total_ = &total;
    start_ = std::chrono::steady_clock::now();
}

HistogramScopedTimer::~HistogramScopedTimer()
{
    if (total_ == nullptr) {
        return;
    }

    const auto elapsed = std::chrono::steady_clock::now() - start_;
    total_->fetch_add(static_cast<unsigned long long>(
                          std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()),
                      std::memory_order_relaxed);
}

HistogramChunkTimer::HistogramChunkTimer(std::atomic<unsigned long long>& total)
{
    if (!gHistogramRuntimeOptions.enableHistogramTimers) {
        return;
    }
    current_ = &total;
    start_ = std::chrono::steady_clock::now();
}

HistogramChunkTimer::~HistogramChunkTimer()
{
    Flush();
}

void HistogramChunkTimer::Next(std::atomic<unsigned long long>& total)
{
    if (current_ == nullptr) {
        return;
    }

    Flush();
    current_ = &total;
    start_ = std::chrono::steady_clock::now();
}

void HistogramChunkTimer::Flush()
{
    if (current_ == nullptr) {
        return;
    }

    const auto elapsed = std::chrono::steady_clock::now() - start_;
    current_->fetch_add(static_cast<unsigned long long>(
                            std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()),
                        std::memory_order_relaxed);
    current_ = nullptr;
}
