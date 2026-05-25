#include "ThreadSafeLookupTable.h"
#include "MyClass.h"
#include <set>
#include <thread>


void TestThreadSafeLookupTable() {
    std::set<int> removeSet;

    threadsafe_lookup_tabel<int, std::shared_ptr<MyClass>> lookupTable;

    std::thread t1([&]() {
        for (int i = 0; i < 100; i++) {
            auto class_ptr = std::make_shared<MyClass>(i);
            lookupTable.add_or_update_mapping(i, class_ptr);
        }
    });

    std::thread t2([&]() {
        for (int i = 0; i < 100;) {
            auto find_res = lookupTable.value_for(i, nullptr);
            if (find_res) {
               lookupTable.remove_mapping(i);
               removeSet.insert(i);
               i++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::thread t3([&]() {
        for (int i = 100; i < 200; i++) {
            auto class_ptr = std::make_shared<MyClass>(i);
            lookupTable.add_or_update_mapping(i, class_ptr);
        }
    });

    t1.join();
    t2.join();
    t3.join();

    for (auto i : removeSet) {
        std::cout << "remove key: " << i << "\n";
    }

    auto copy_map = lookupTable.get_map();
    for (auto& pair : copy_map) {
        std::cout << "key: " << pair.first << ", value: " << *(pair.second) << "\n";
    }
}

int main() {
    TestThreadSafeLookupTable();
    return 0;
}