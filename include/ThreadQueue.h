#ifndef hThreadQueue
#define hThreadQueue

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>

#include <Globals.h>
#include <BuiltEvent.h>

inline std::atomic<size_t> g_buffer_size{0};
inline std::atomic<size_t> g_idx{0};
inline std::atomic<size_t> g_ReadCount{0};
inline std::atomic<size_t> g_BuiltCount{0};
inline std::atomic<size_t> g_QueuedBuiltEvents{0};
inline std::atomic<int> g_refill_state_a{0};
inline std::atomic<int> g_refill_state_b{0};

template<typename T>
class ThreadSafeQueue {
    std::queue<T> q;                  // Underlying FIFO queue holding data
    mutable std::mutex m;                     // Mutex to protect access to q and finished
    std::condition_variable cv_not_empty; // consumer waits on this
    std::condition_variable cv_not_full;  // producer waits on this
    size_t max_size;   // maximum number of items allowed
    bool finished = false;            // Signals "no more data will ever be added"
    
public:
    ThreadSafeQueue(size_t maxSize) : max_size(maxSize) {}
    
    // =========================
    // PUSH (Producer)
    // =========================
    void push(T val) {
        // Lock the mutex so only one thread modifies the queue at a times
        // unique_lock is required (not lock_guard) because:
        // - condition_variable::wait needs to temporarily unlock the mutex
        std::unique_lock<std::mutex> lock(m);
        
        // Wait until queue has space
        cv_not_full.wait(lock, [&] {
            return q.size() < max_size || finished;
        });
        
        // Optional: allow early exit if finished was set externally
        if (finished) return;
        
        q.push(std::move(val));
        
        // Notify ONE consumer that data is available (only using 1)
        cv_not_empty.notify_one();
    }
    
    // =========================
    // SIGNAL COMPLETION
    // =========================
    void set_finished() {
        std::lock_guard<std::mutex> lock(m);
        finished = true;
        // Indicate that no more data will be pushed
        
        // Wake everyone so they can exit
        cv_not_empty.notify_all();
        cv_not_full.notify_all();
    }
    
    // =========================
    // POP (Consumer)
    // =========================
    bool pop(T& out) {
        std::unique_lock<std::mutex> lock(m);
        
        // Wait until data is available OR finished
        cv_not_empty.wait(lock, [&] {
            return !q.empty() || finished;
        });
        
        if (!q.empty()) {
            // Data is available → consume it
            out = std::move(q.front());// move avoids copying
            q.pop();
            
            // Notify ONE producer that space is available
            cv_not_full.notify_one();
            
            return true;
        }
        
        // If we reach here:
        //   queue is empty AND finished == true
        //
        // This means:
        //   - producer has finished
        //   - no more data will ever arrive
        //
        // This is the ONLY place we return false
        return false;
    }
    
    // =========================
    // Data to Monitor thread
    // =========================
    size_t size() const {
        std::lock_guard<std::mutex> lock(m);
        return q.size();
    }
};


std::string make_bar(size_t value, size_t max, size_t width = 30);

std::string make_buffer_bar(size_t size,size_t max,size_t idx,size_t width = 24);

std::string make_queue_bar(size_t size,size_t max, size_t width = 20);

void QueueMonitorThread(ThreadSafeQueue<std::vector<Event>>& queue, size_t MAX_QUEUE, size_t BUFFERSIZE, std::atomic<bool>& done_flag);

void BuildMonitorThread(size_t builtEventBudget,
                        size_t bufferSize,
                        size_t refillTarget,
                        std::atomic<bool>& done_flag);

#endif
