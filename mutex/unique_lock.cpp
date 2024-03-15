#include <iostream>
#include <mutex>
#include <thread>
#include <shared_mutex>
#include <map>

std::mutex mtx;
int shared_data = 0;
// 查看是否占有锁，手动解锁
void use_unique_owns() {
    std::unique_lock<std::mutex> guard(mtx);
    if (guard.owns_lock()){
        std::cout << "owns lock" << std::endl;
    }
    else {
        std::cout << "doesn't own lock" << std::endl;
    }
    shared_data++;
    guard.unlock();
    if (guard.owns_lock()){
        std::cout << "owns lock" << std::endl;
    }
    else {
        std::cout << "doesn't own lock" << std::endl;
    }
}
// 延迟加锁, 可以加锁，自动析构解锁，也可以手动解锁
void defer_unique() {
    std::unique_lock<std::mutex> guard(mtx, std::defer_lock);
    guard.lock();
    guard.unlock();
}
// 支持领养
void adopt_unique() {
    mtx.lock();
    std::unique_lock<std::mutex> guard(mtx, std::adopt_lock);
    if (guard.owns_lock()){
        std::cout << "owns lock" << std::endl;
    }
    else {
        std::cout << "doesn't own lock" << std::endl;
    }
    guard.unlock();
}

int a = 10, b = 100;
std::mutex mtx1;
std::mutex mtx2;

void safe_swap_adopt() {
    std::lock(mtx1, mtx2);
    std::unique_lock<std::mutex> guard1(mtx1, std::adopt_lock); 
    std::unique_lock<std::mutex> guard2(mtx2, std::adopt_lock); 
    std::swap(a,b);
    guard1.unlock(); // 可自动释放， 已经领养不能mtx1.unlock()
    guard2.unlock();
    std::cout << "a = " << a << ", b = " << b << std::endl;
}

void safe_swap_defer() {
    std::unique_lock<std::mutex> guard1(mtx1, std::defer_lock); 
    std::unique_lock<std::mutex> guard2(mtx2, std::defer_lock); 
    std::lock(guard1, guard2);
    std::swap(a,b);
    std::cout << "a = " << a << ", b = " << b << std::endl;
}

std::unique_lock<std::mutex> get_lock() {
    std::unique_lock<std::mutex> lk(mtx);
    shared_data++;
    return lk;
}
void test_return() {
    std::unique_lock<std::mutex> lk(get_lock());
    shared_data++;
}

void precision_lock() {
    std::unique_lock<std::mutex> lk(mtx);
    shared_data++;
    lk.unlock();
   // 不涉及共享数据的耗时操作不在锁内执行;
   std::this_thread::sleep_for(std::chrono::seconds(1));
    lk.lock();
    shared_data++;
    lk.unlock();
}

// DNS缓存
class dns_cache {
public:
    std::string  find_entry(std::string const & domain) const {
        std::shared_lock<std::shared_mutex> lk(entry_mutex); // 保护共享和只读权限
        std::map<std::string, std::string>::const_iterator const it = entries.find(domain);
        return (it == entries.end()) ? "" : it->second;
    }
    void update_or_add_entry(std::string const & domain, std::string const& dns_details) {
        std::lock_guard<std::shared_mutex> lk(entry_mutex);
        entries[domain] = dns_details;
    }
private:
    std::map<std::string, std::string> entries;
    mutable std::shared_mutex entry_mutex;
};

void test_dns_cache() {
    dns_cache cache;
    // 添加条目到缓存
    cache.update_or_add_entry("www.example.com", "192.168.1.1");

    // 在不同线程中查找条目
    std::thread t1([&cache](){
        std::string result = cache.find_entry("www.example.com");
        std::cout << "Thread 1 - IP address for www.example.com: " << result << std::endl;
    });

    std::thread t2([&cache](){
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待1秒
        std::string result = cache.find_entry("www.example.com");
        std::cout << "Thread 2 - IP address for www.example.com: " << result << std::endl;
    });

    // 等待线程完成
    t1.join();
    t2.join();
}

class RecursiveDemo {
public:
    RecursiveDemo() {}
    bool QueryStudent(std::string name) {
        // std::lock_guard<std::mutex> mutex_lock(_mtx);
        std::lock_guard<std::recursive_mutex> recursive_lock(_recursive_mtx);
        auto iter_find = _students_info.find(name);
        if (iter_find == _students_info.end()) {
            return false;
        }
        return true;
    }
    void AddScore(std::string name, int score) {
        // std::lock_guard<std::mutex> mutex_lock(_mtx);
        std::lock_guard<std::recursive_mutex>  recursive_lock(_recursive_mtx);
        if (!QueryStudent(name)) {
            _students_info.insert(std::make_pair(name, score));
            return;
        }
        _students_info[name] = _students_info[name] + score;
    }
    void AddScoreAtomic(std::string name, int score) {
        std::lock_guard<std::mutex> mutex_lock(_mtx);
        // std::lock_guard<std::recursive_mutex>  recursive_lock(_recursive_mtx);
        auto iter_find = _students_info.find(name);
        if (iter_find == _students_info.end()){
             _students_info.insert(std::make_pair(name, score));
            return;
        }
         _students_info[name] = _students_info[name] + score;
        return;
    }
private:
    std::map<std::string, int> _students_info;
    std::mutex _mtx;
    std::recursive_mutex _recursive_mtx;
};

void test_recursive_demo() {
    RecursiveDemo demo;
    std::thread t1([&demo](){
        // demo.AddScore("Alice", 10);
        demo.AddScoreAtomic("Alice", 10);
    });
    std::thread t2([&demo](){
        // demo.AddScore("Bob", 20);
         demo.AddScoreAtomic("Bob", 20);
    });
    t1.join();
    t2.join();
}
int main() {
    // use_unique_owns();
    // defer_unique();
    // adopt_unique();
    // safe_swap_adopt();
    // safe_swap_defer();
    // test_return();
    // precision_lock();
    // test_dns_cache();
    test_recursive_demo();
    return 0;
}