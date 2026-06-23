#include <cassert>
#include <chrono>
#include <mutex>
#include <set>
#include <thread>

#include "LockFreeStack.h"
#include "LockFreeStackHP.h"
#include "LockFreeStackRefCount.h"
#include "LockFreeStackMemory.h"

template <typename Stack>
void TestLockFreeStack() {
    Stack lk_free_stack;
    std::set<int> rmv_set;
    std::mutex set_mtx;

    std::thread t1([&](){
        for (int i = 0; i < 20000; ++i) {
            lk_free_stack.push(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    std::thread t2([&](){
        for (int i = 0; i < 10000;) {
            auto head = lk_free_stack.pop();
            if (!head) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            std::lock_guard<std::mutex> lock(set_mtx);
            rmv_set.insert(*head);
            i++;
        }
    });

    std::thread t3([&](){
        for (int i = 0; i < 10000;) {
            auto head = lk_free_stack.pop();
            if (!head) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            std::lock_guard<std::mutex> lock(set_mtx);
            rmv_set.insert(*head);
            i++;
        }
    });

    t1.join();
    t2.join();
    t3.join();

    assert(rmv_set.size() == 20000);
}

int main() {
    TestLockFreeStack<lockfree_stack<int>>();
    printf("TestLockFreeStack<lockfree_stack<int>>\n");
    TestLockFreeStack<lockfree_stack_hp<int>>();
    printf("TestLockFreeStack<lockfree_stack_hp<int>>\n");
    TestLockFreeStack<lockfree_stack_refcount<int>>();
    printf("TestLockFreeStack<lockfree_stack_refcount<int>>\n");
    TestLockFreeStack<lockfree_stack_memory<int>>();
    printf("TestLockFreeStack<lockfree_stack_memory<int>>\n");
    return 0;
}
