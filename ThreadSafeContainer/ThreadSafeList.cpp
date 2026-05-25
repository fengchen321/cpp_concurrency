#include "ThreadSafeList.h"
#include "MyClass.h"
#include <set>
#include <thread>


void TestThreadSafeList() {
    std::set<int> removeSet;

    threadsafe_list<MyClass> thread_safe_list;

    std::thread t1([&]() {
        for (int i = 0; i < 100; i++) {
            MyClass mc(i);
            thread_safe_list.push_front(mc);
        }
    });

    std::thread t2([&]() {
        for (int i = 0; i < 100;) {
            auto find_res = thread_safe_list.find_first_if([&](const MyClass& mc) { return mc.GetData() == i; });
            if  (find_res == nullptr) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            removeSet.insert(i);
            i++;
        }
    });

    t1.join();
    t2.join();

    for (auto i : removeSet) {
        std::cout << "remove key: " << i << "\n";
    }
}

int main() {
    TestThreadSafeList();
    return 0;
}