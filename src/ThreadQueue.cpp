#include <ThreadQueue.h>


std::string make_bar(size_t value, size_t max, size_t width) {
    if (max == 0) return std::string(width, '-');
    
    size_t filled = (value * width) / max;
    if (filled > width) filled = width;
    
    return std::string(filled, '#') + std::string(width - filled, '-');
}

std::string make_buffer_bar(size_t size,
                            size_t max,
                            size_t idx,
                            size_t pop,
                            size_t width)
{
    std::string bar(width, ' ');  // unused = space
    
    if (max == 0) return bar;
    
    // Clamp inputs safely
    size = std::min(size, max);
    idx  = std::min(idx, size);
    pop  = std::min(pop, idx);  // ensures (idx - pop) >= 0
    
    size_t pop_start = idx - pop;
    
    auto scale = [&](size_t v) {
        size_t pos = (v * width) / max;
        return (pos >= width) ? width - 1 : pos;
    };
    
    size_t size_pos = scale(size);
    size_t idx_pos  = scale(idx);
    size_t pop_pos  = scale(pop_start);
    
    for (size_t i = 0; i < width; i++) {
        
        if (i < pop_pos) {
            bar[i] = '-';   // already processed
        }
        else if (i < idx_pos) {
            bar[i] = '=';   // pop window (before idx)
        }
        else if (i < size_pos) {
            bar[i] = '#';   // unprocessed buffer
        }
        else {
            bar[i] = ' ';   // unused
        }
    }
    
    return bar;
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

void QueueMonitorThread(ThreadSafeQueue<std::vector<Event>>& queue,
             size_t MAX_QUEUE,
             size_t BUFFERSIZE,
             std::atomic<bool>& done_flag)
{
    using namespace std::chrono_literals;
    
    const size_t BUFFER_MAX = BUFFERSIZE * 2;
    
    int w1 = std::to_string(MAX_QUEUE).size();
    
    while (!done_flag.load()) {
        
        size_t qsize = queue.size();
        size_t bsize = g_buffer_size.load();
        size_t idx   = g_idx.load();
        size_t pop   = g_popCount.load();
        size_t read   = g_ReadCount.load();
        size_t built   = g_BuiltCount.load();
        
        std::string qbar = make_queue_bar(qsize, MAX_QUEUE,10);
        std::string bbar = make_buffer_bar(bsize, BUFFER_MAX, idx, pop);
        
        // Colour logic (simple)
        const char* qcolor = (qsize > MAX_QUEUE * 0.8) ? CLR_RED : CLR_GREEN;
        const char* bcolor = (bsize > BUFFER_MAX * 0.8) ? CLR_YELLOW : CLR_CYAN;
        
        {
            std::cout
//             << "\r"
            << "\r\033[K"  // carriage return + clear line
            << "Q " << qcolor << "[" << qbar << "]" << CLR_RESET
            << std::setw(w1) << qsize
//             << " " << qsize << "/" << MAX_QUEUE
            << " | B " << bcolor << "[" << bbar << "]" << CLR_RESET
//             << " " << bsize << "/" << BUFFER_MAX
//             << " idx:" << idx
//             << " pop:" << pop
            <<read<< ":" << built
//             << "      "   // padding to clear leftovers
            << std::flush;
        }
        
        std::this_thread::sleep_for(400ms);
    }
    
    std::cout << std::endl;
}


