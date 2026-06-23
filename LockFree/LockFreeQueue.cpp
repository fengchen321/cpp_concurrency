#include <cassert>
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

#include "LockFreeQueueSingle.h"
#include "LockFreeQueue_0.h"
#include "LockFreeQueue.h"

#define TESTCOUNT 10000

template <typename Queue>
void TestQueueSequential() {
    Queue queue;

    assert(!queue.pop());

    for (int i = 0; i < TESTCOUNT; ++i) {
        queue.push(i);
    }

    for (int i = 0; i < TESTCOUNT; ++i) {
        auto value = queue.pop();
        assert(value);
        assert(*value == i);
    }

    assert(!queue.pop());
}

template <typename Queue>
void TestQueueConcurrent() {
    constexpr int producer_count = 2;
    constexpr int consumer_count = 2;
    constexpr int per_producer = TESTCOUNT;
    constexpr int total_count = producer_count * per_producer;

    Queue queue;
    std::vector<std::atomic<int>> seen(total_count);
    for (auto& count : seen) {
        count.store(0, std::memory_order_relaxed);
    }

    std::atomic<int> producers_done{0};
    std::atomic<int> consumed{0};
    std::atomic<bool> bad_value{false};
    std::atomic<bool> stalled{false};
    std::atomic<int> empty_after_done{0};

    std::vector<std::thread> producers;
    for (int producer = 0; producer < producer_count; ++producer) {
        producers.emplace_back([&, producer] {
            const int begin = producer * per_producer;
            const int end = begin + per_producer;
            for (int value = begin; value < end; ++value) {
                queue.push(value);
            }
            producers_done.fetch_add(1, std::memory_order_release);
        });
    }

    std::vector<std::thread> consumers;
    for (int consumer = 0; consumer < consumer_count; ++consumer) {
        consumers.emplace_back([&] {
            for (;;) {
                if (stalled.load(std::memory_order_acquire)) {
                    break;
                }
                if (consumed.load(std::memory_order_acquire) >= total_count &&
                    producers_done.load(std::memory_order_acquire) == producer_count) {
                    break;
                }

                auto value = queue.pop();
                if (!value) {
                    if (producers_done.load(std::memory_order_acquire) == producer_count &&
                        empty_after_done.fetch_add(1, std::memory_order_relaxed) > 1000000) {
                        stalled.store(true, std::memory_order_release);
                    }
                    std::this_thread::yield();
                    continue;
                }

                empty_after_done.store(0, std::memory_order_relaxed);
                if (*value < 0 || *value >= total_count) {
                    bad_value.store(true, std::memory_order_relaxed);
                } else {
                    seen[*value].fetch_add(1, std::memory_order_relaxed);
                }
                consumed.fetch_add(1, std::memory_order_release);
            }
        });
    }

    for (auto& thread : producers) {
        thread.join();
    }
    for (auto& thread : consumers) {
        thread.join();
    }

    int missing = 0;
    int duplicated = 0;
    for (int value = 0; value < total_count; ++value) {
        int const count = seen[value].load(std::memory_order_relaxed);
        if (count == 0) {
            ++missing;
        } else if (count > 1) {
            duplicated += count - 1;
        }
    }

    if (stalled.load(std::memory_order_relaxed) ||
        bad_value.load(std::memory_order_relaxed) ||
        consumed.load(std::memory_order_relaxed) != total_count ||
        missing != 0 ||
        duplicated != 0) {
        std::printf("concurrent test failed: producers_done=%d consumed=%d missing=%d duplicated=%d bad_value=%d stalled=%d\n",
            producers_done.load(std::memory_order_relaxed),
            consumed.load(std::memory_order_relaxed),
            missing,
            duplicated,
            bad_value.load(std::memory_order_relaxed),
            stalled.load(std::memory_order_relaxed));
        std::fflush(stdout);
    }

    assert(!stalled.load(std::memory_order_relaxed));
    assert(!bad_value.load(std::memory_order_relaxed));
    assert(consumed.load(std::memory_order_relaxed) == total_count);
    for (int value = 0; value < total_count; ++value) {
        assert(seen[value].load(std::memory_order_relaxed) == 1);
    }
    assert(!queue.pop());
}

int main() {
    std::printf("lockfreequeuesingle sequential start\n");
    std::fflush(stdout);
    TestQueueSequential<lockfreequeuesingle::lockfree_queue<int>>();
    std::printf("lockfreequeuesingle sequential done\n");
    std::fflush(stdout);

    std::printf("lockfreequeue0 sequential start\n");
    std::fflush(stdout);
    TestQueueSequential<lockfreequeue0::lockfree_queue<int>>();
    std::printf("lockfreequeue0 sequential done\n");
    std::fflush(stdout);

    std::printf("lockfreequeue0 concurrent start\n");
    std::fflush(stdout);
    TestQueueConcurrent<lockfreequeue0::lockfree_queue<int>>();
    std::printf("lockfreequeue0 concurrent done\n");
    std::fflush(stdout);

    std::printf("lockfreequeue1 sequential start\n");
    std::fflush(stdout);
    TestQueueSequential<lockfreequeue1::lockfree_queue<int>>();
    std::printf("lockfreequeue1 sequential done\n");
    std::fflush(stdout);

    // todo: for i in {1..1000}; do ./LockFree/LockFreeQueue; done 
    // queue 有问题，需要调试
    std::printf("lockfreequeue1 concurrent start\n");
    std::fflush(stdout);
    TestQueueConcurrent<lockfreequeue1::lockfree_queue<int>>();
    std::printf("lockfreequeue1 concurrent done\n");
    std::fflush(stdout);

    return 0;
}
