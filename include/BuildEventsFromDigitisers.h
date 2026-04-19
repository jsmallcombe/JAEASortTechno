#ifndef JAEASortBuildEventsFromDigitisers
#define JAEASortBuildEventsFromDigitisers

#include <BuiltEvent.h>
#include <Digitisers.h>
#include <Globals.h>
#include <ThreadQueue.h>

#include <algorithm>
#include <array>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>



// Shared raw-event builder used by the threaded tree-only and tree+histogram
// paths. The builder owns the double-buffer refill coordination internally,
// while callers only provide the built-event sink callback.
template <typename EventWriter>
void BuildEventsFromDigitisers(std::vector<std::unique_ptr<DigitiserBase>>& digitisers,
                               Long64_t tdiff,
                               size_t BUFFER,
                               size_t CHUNK_SIZE,
                               BuiltEvent& eventBuffer,
                               EventWriter&& writeEvent,
                               DigitiserAdcHistograms* ADChists = nullptr)
{
    const Long64_t COINC_WINDOW = tdiff;
    const size_t BUFFER_TARGET = BUFFER;
    const size_t REFILL_TARGET = std::max<size_t>(1, BUFFER / gBuildRefillDivisor);

    struct RefillSlot {
        std::vector<Event> events;
        enum class State {
            Free,
            Filling,
            Sorting,
            Ready
        } state = State::Free;
    };

    struct RefillCoordinator {
        std::mutex mutex;
        std::condition_variable cv;
        std::array<RefillSlot, 2> slots;
        std::deque<int> readyOrder;
        bool producerDone = false;
    };

    Event ev;
    Long64_t globalMaxTs = -1;
    bool inputFinished = false;

    auto SetRefillState = [&](int slotIndex, int stateValue) {
        if (slotIndex < 0) {
            return;
        }
        if (slotIndex == 0) {
            g_refill_state_a = stateValue;
        } else {
            g_refill_state_b = stateValue;
        }
    };

    auto FillSortedBlock = [&](int slotIndex, std::vector<Event>& target, size_t targetSize) {
        target.clear();
        target.reserve(std::max(target.capacity(), targetSize));

        size_t added = 0;
        while (added < targetSize && !inputFinished) {
            bool anyActive = false;

            for (auto& digiPtr : digitisers) {
                auto& digi = *digiPtr;

                bool accepted = false;
                while (!accepted) {
                    const size_t sizeBefore = target.size();
                    int count = 0;

                    while (digi.getNextEvent(ev)) {
                        if (ADChists) {
                            ADChists->Fill(ev.mod, ev.ch, ev.adc);
                        }
                        target.push_back(ev);
                        ++count;
                        if (count >= static_cast<int>(CHUNK_SIZE)) {
                            break;
                        }
                    }

                    if (target.size() == sizeBefore) {
                        break;
                    }

                    anyActive = true;
                    added += target.size() - sizeBefore;

                    const Long64_t digiTs = digi.getLastTs();
                    if (globalMaxTs < 0 || digiTs >= globalMaxTs) {
                        globalMaxTs = digiTs;
                        accepted = true;
                    }

                    if (added >= targetSize && accepted) {
                        break;
                    }
                }

                if (added >= targetSize) {
                    break;
                }
            }

            if (!anyActive) {
                inputFinished = true;
            }
        }

        SetRefillState(slotIndex, static_cast<int>(RefillSlot::State::Sorting));
        std::sort(target.begin(),
                  target.end(),
                  [](const Event& a, const Event& b) {
                      return a.ts < b.ts;
                  });

        return target.size();
    };

    std::vector<Event> buildBuffer;
    buildBuffer.reserve(BUFFER_TARGET);
    size_t readCount = FillSortedBlock(-1, buildBuffer, BUFFER_TARGET);
    g_ReadCount = readCount;

    if (buildBuffer.empty()) {
        g_buffer_size = 0;
        g_idx = 0;
        g_BuiltCount = 0;
        return;
    }

    RefillCoordinator refills;

    std::thread consumerThread([&]() {
        Long64_t firstTs = -1;
        Long64_t lastTs = -1;
        size_t idx = 0;
        size_t sinceMerge = 0;
        size_t FILL_EXCESS = 0;
        size_t builtCount = 0;

        auto MergeNextRefill = [&]() {
            int slotIndex = -1;
            {
                std::unique_lock<std::mutex> lock(refills.mutex);
                refills.cv.wait(lock, [&]() {
                    return !refills.readyOrder.empty() || refills.producerDone;
                });

                if (refills.readyOrder.empty()) {
                    return static_cast<size_t>(0);
                }

                slotIndex = refills.readyOrder.front();
                refills.readyOrder.pop_front();
            }

            auto& refillBuffer = refills.slots[slotIndex].events;
            const size_t added = refillBuffer.size();
            const size_t oldSize = buildBuffer.size() - idx;
            buildBuffer.insert(buildBuffer.end(),
                               std::make_move_iterator(refillBuffer.begin()),
                               std::make_move_iterator(refillBuffer.end()));

            const auto base = buildBuffer.begin() + idx;
            std::inplace_merge(base,
                               base + oldSize,
                               buildBuffer.end(),
                               [](const Event& a, const Event& b) {
                                   return a.ts < b.ts;
                               });

            refillBuffer.clear();

            {
                std::lock_guard<std::mutex> lock(refills.mutex);
                refills.slots[slotIndex].state = RefillSlot::State::Free;
            }
            SetRefillState(slotIndex, static_cast<int>(RefillSlot::State::Free));
            refills.cv.notify_all();
            return added;
        };

        // The consumer owns the active ordered buffer and the current
        // coincidence event state. It only pauses to merge a refill block
        // that the producer has already read and locally sorted.
        while (idx < buildBuffer.size()) {
            Event& current = buildBuffer[idx++];
            ++sinceMerge;

            const Long64_t currentTs = current.ts;

            if (eventBuffer.Empty()) {
                firstTs = currentTs;
                lastTs = currentTs;
                eventBuffer.StartEvent(current);
            } else if (currentTs < lastTs) {
                std::cout << "\n[TIME RESET]\n";
                firstTs = currentTs;
                lastTs = currentTs;
                eventBuffer.StartEvent(current);
            } else if (currentTs - lastTs < COINC_WINDOW) {
                eventBuffer.AppendHit(current, firstTs);
                lastTs = currentTs;
            } else {
                writeEvent(eventBuffer);
                ++builtCount;

                firstTs = currentTs;
                lastTs = currentTs;
                eventBuffer.StartEvent(current);
            }

            if (sinceMerge >= REFILL_TARGET + FILL_EXCESS) {
                const size_t merged = MergeNextRefill();
                if (merged > REFILL_TARGET) {
                    FILL_EXCESS = merged - REFILL_TARGET;
                }
                sinceMerge = 0;

                // Compact only occasionally so the consumer keeps working on
                // one contiguous active region instead of shifting on every merge.
                if (buildBuffer.size() > 2 * BUFFER) {
                    buildBuffer.erase(buildBuffer.begin(), buildBuffer.begin() + idx);
                    idx = 0;
                }
            }

            if ((idx % 1000) == 0) {
                g_buffer_size = buildBuffer.size();
                g_idx = idx;
                g_BuiltCount = builtCount;
            }
        }

        if (!eventBuffer.Empty()) {
            writeEvent(eventBuffer);
            ++builtCount;
        }

        g_buffer_size = buildBuffer.size();
        g_idx = idx;
        g_BuiltCount = builtCount;
    });

    // The main thread becomes the producer after the initial full buffer is
    // prepared. It alternates between two refill vectors so the next block can
    // be read and sorted while the consumer is merging the previous one.
    while (true) {
        int slotIndex = -1;
        {
            std::unique_lock<std::mutex> lock(refills.mutex);
            refills.cv.wait(lock, [&]() {
                for (const RefillSlot& slot : refills.slots) {
                    if (slot.state == RefillSlot::State::Free) {
                        return true;
                    }
                }
                return false;
            });

            for (size_t i = 0; i < refills.slots.size(); ++i) {
                if (refills.slots[i].state == RefillSlot::State::Free) {
                    refills.slots[i].state = RefillSlot::State::Filling;
                    slotIndex = static_cast<int>(i);
                    break;
                }
            }
        }

        if (slotIndex < 0) {
            continue;
        }

        RefillSlot& slot = refills.slots[slotIndex];
        SetRefillState(slotIndex, static_cast<int>(RefillSlot::State::Filling));
        const size_t added = FillSortedBlock(slotIndex, slot.events, REFILL_TARGET);
        readCount += added;
        g_ReadCount = readCount;

        {
            std::lock_guard<std::mutex> lock(refills.mutex);
            if (slot.events.empty()) {
                slot.state = RefillSlot::State::Free;
                refills.producerDone = true;
            } else {
                slot.state = RefillSlot::State::Ready;
                refills.readyOrder.push_back(slotIndex);
            }
        }
        SetRefillState(slotIndex,
                       slot.events.empty()
                           ? static_cast<int>(RefillSlot::State::Free)
                           : static_cast<int>(RefillSlot::State::Ready));
        refills.cv.notify_all();

        if (slot.events.empty()) {
            break;
        }
    }

    consumerThread.join();
}

#endif
