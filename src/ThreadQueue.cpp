#include <ThreadQueue.h>
#include <sstream>


std::string make_bar(size_t value, size_t max, size_t width) {
    if (max == 0) return std::string(width, '-');
    
    size_t filled = (value * width) / max;
    if (filled > width) filled = width;
    
    return std::string(filled, '#') + std::string(width - filled, '-');
}

std::string make_buffer_bar(size_t size,
                            size_t max,
                            size_t idx,
                            size_t width)
{
    std::string bar(width, ' ');
    
    if (max == 0) return bar;
    
    size = std::min(size, max);
    idx  = std::min(idx, size);
    
    auto scale = [&](size_t v) {
        if (width == 0) return static_cast<size_t>(0);
        size_t pos = (v * width) / max;
        if (pos >= width && v < max) {
            pos = width - 1;
        }
        return (pos > width) ? width : pos;
    };
    
    size_t size_pos = scale(size);
    size_t idx_pos  = scale(idx);
    
    for (size_t i = 0; i < width; i++) {
        if (i < idx_pos) {
            bar[i] = '-';   // already built/consumed
        } else if (i < size_pos) {
            bar[i] = '#';   // buffered and waiting
        } else {
            bar[i] = ' ';   // unused capacity
        }
    }
    
    return bar;
}

const char* refill_state_label(int state)
{
    switch (state) {
        case 1: return "F";
        case 2: return "S";
        case 3: return "R";
        default: return "-";
    }
}

std::string make_queue_bar(size_t size,
                           size_t max,
                           size_t width)
{
    std::string bar(width, '-');
    
    if (max == 0) return bar;
    
    size_t filled = (size * width) / max;
    if (filled > width) filled = width;
    
    for (size_t i = 0; i < filled; i++)
        bar[i] = '#';
    
    return bar;
}

std::string format_thousands(size_t value)
{
    std::ostringstream os;
    os << std::fixed << std::setprecision(1)
       << (static_cast<double>(value) / 1.0e3) << "k";
    return os.str();
}

size_t dynamic_queue_bar_width(size_t maxSeen)
{
    return std::min<size_t>(10, maxSeen);
}

void BuildMonitorThread(size_t builtEventBudget,
             size_t bufferSize,
             size_t refillTarget,
             std::atomic<bool>& done_flag)
{
    using namespace std::chrono_literals;

    const size_t bufferMax = (bufferSize * 2 > refillTarget)
        ? (bufferSize * 2 - refillTarget)
        : bufferSize;
    const bool showBuiltQueue = builtEventBudget > 0;
    size_t queuedBuiltMaxSeen = 1;

    while (!done_flag.load()) {
        const size_t bsize = g_buffer_size.load();
        const size_t idx   = g_idx.load();
        const size_t read  = g_ReadCount.load();
        const size_t built = g_BuiltCount.load();
        const size_t queuedBuilt = g_QueuedBuiltEvents.load();
        const int refillStateA = g_refill_state_a.load();
        const int refillStateB = g_refill_state_b.load();

        if (queuedBuilt > queuedBuiltMaxSeen) {
            queuedBuiltMaxSeen = queuedBuilt;
        }

        const std::string bbar = make_buffer_bar(bsize, bufferMax, idx, 15);
        const std::string ebar = make_queue_bar(queuedBuilt,
                                                queuedBuiltMaxSeen,
                                                dynamic_queue_bar_width(queuedBuiltMaxSeen));

        const char* bcolor = (bsize > bufferMax * 0.8) ? CLR_YELLOW : CLR_CYAN;
        const char* ecolor = (queuedBuiltMaxSeen > 0 && queuedBuilt > queuedBuiltMaxSeen * 0.8)
            ? CLR_RED
            : CLR_GREEN;

        std::cout
            << "\r\033[K"
            << "B " << bcolor << "["
            << refill_state_label(refillStateA) << ":"
            << refill_state_label(refillStateB) << ":"
            << bbar << "]" << CLR_RESET;

        if (showBuiltQueue) {
            std::cout
                << " | E " << ecolor << "[" << ebar << "]" << CLR_RESET
                << format_thousands(queuedBuiltMaxSeen);
        }

        std::cout
            << " | " << read << ":" << built
            << std::flush;

        std::this_thread::sleep_for(200ms);
    }

    std::cout << std::endl;
}
