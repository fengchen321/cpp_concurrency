#include <thread>
#include "MyClass.h"
#include "CircularQueLk.h"
#include "CircularQueSeq.h"
#include "CircularQueLight.h"
#include "CircularQueSync.h"
// 无法推导出T和Cap
template<typename QueueType, typename T, size_t Cap>
void test_circular_queue() {
    QueueType cq;

    T mc_1(1);
    T mc_2(2);

    cq.push(mc_1);
    cq.push(std::move(mc_2));
    for (int i = 3; i <= static_cast<int>(Cap); i++) {
        T mc(i);
        auto res = cq.push(mc);
        if (!res) {
            break;
        }
    }
    cq.push(mc_2);
    for (int i = 0; i < static_cast<int>(Cap); i++) {
        T mc_1;
        auto res = cq.pop(mc_1);
        if (!res) {
            break;
        }
        std::cout << "pop success, " << mc_1 << "\n";
    }
    cq.pop(mc_1);
}

template<typename QueueType, typename T, size_t Cap>
void test_circular_queue_multithread() {
    QueueType queue;
    std::atomic<bool> stopConsumer{false}; 

    std::thread producerThread([&](){
        for (int i = 0; i < static_cast<int>(Cap); ++i) {
            T item(i);
            queue.push(item);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            std::cout << "Producer added item with data " << i << std::endl;
        }
    });

    std::thread consumerThread([&](){
        T item;
        while (!stopConsumer.load()) {
            if (queue.pop(item)) {
                std::cout << "Consumer received item with data " << item << std::endl;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 队列空时等待
            }
        }
    });
    producerThread.join();
    stopConsumer.store(true);
    consumerThread.join();
}

int main() {
    // test_circular_queue<CircularQueLk<MyClass, 5>, MyClass, 5>();
    // test_circular_queue<CircularQueSeq<MyClass, 5>, MyClass, 5>();
    // test_circular_queue<CircularQueLight<MyClass, 5>, MyClass, 5>();
    // test_circular_queue<CircularQueSync<MyClass, 5>, MyClass, 5>();
    // test_circular_queue_multithread<CircularQueLk<MyClass, 10>, MyClass, 10>();
    // test_circular_queue_multithread<CircularQueSeq<MyClass, 10>, MyClass, 10>();
    // test_circular_queue_multithread<CircularQueLight<MyClass, 10>, MyClass, 10>();
    test_circular_queue_multithread<CircularQueSync<MyClass, 10>, MyClass, 10>();
    return 0;
}