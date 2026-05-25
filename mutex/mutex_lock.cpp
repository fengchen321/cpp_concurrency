#include <iostream>
#include <thread>
#include <mutex>
#include <stack>
#include <exception>
#include <climits>
#include <memory>

int shared_data = 100;
std::mutex mtx1;

void use_lock(){
    while(true) {
        mtx1.lock();
        shared_data++;
        std::cout << "current thread is " << std::this_thread::get_id()
                  << ", shard data id " << shared_data << std::endl;
        mtx1.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

void auto_use_lock(){
    while(true) {
       std::lock_guard<std::mutex> lock(mtx1);
        shared_data++;
        std::cout << "current thread is " << std::this_thread::get_id()
                  << ", shard data id " << shared_data << std::endl;
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

void test_lock(){
    // std::thread t1(use_lock);
    std::thread t1(auto_use_lock);
    std::thread t2([](){
        while(true) {
        mtx1.lock();
        shared_data--;
        std::cout << "current thread is " << std::this_thread::get_id()
                  << ", shard data id " << shared_data << std::endl;
        mtx1.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    });
    t1.join();
    t2.join();
}

struct empty_stack: std::exception
{
    const char* what() const throw(){
        return "empty stack!";
    };
};

template<typename T>
class threadsafe_stack
{
private:
    std::stack<T> data;
    mutable std::mutex m;
public:
    threadsafe_stack(){}
    threadsafe_stack(const threadsafe_stack& other){
        std::lock_guard<std::mutex>lock(other.m);
        data = other.data;
    }
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;// 栈不能被赋值

    void push(T new_value){
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));
    }
    // 原始pop + 异常抛出 （仍然有问题T为vector<int>拷贝赋值时造成失败）
    // T pop(){
    //     std::lock_guard<std::mutex> lock(m);
    //     if (data.empty()) throw empty_stack();
    //     auto element = data.top();
    //     data.pop();
    //     return element;
    // }
    // 选项1：传入引用   减少了数据的拷贝，防止内存溢出
    void pop(T& value){
        std::lock_guard<std::mutex> lock(m);
        if (data.empty()) throw empty_stack();
        value = data.top();
        data.pop();
    }

     // 选项3：返回指向栈顶的指针
    std::shared_ptr<T> pop(){
        std::lock_guard<std::mutex> lock(m);
        if (data.empty()) throw empty_stack();
        std::shared_ptr<T> const res(std::make_shared<T>(data.top()));
        data.pop();
        return res;
    }
    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};

void test_threadsafe_stack(){
    threadsafe_stack<int> safe_stack;
    safe_stack.push(1);

    std::thread t1([&safe_stack](){
        if (!safe_stack.empty()){
            std::this_thread::sleep_for(std::chrono::seconds(1));
            safe_stack.pop();
        }
    });

    std::thread t2([&safe_stack](){
        if (!safe_stack.empty()){
            std::this_thread::sleep_for(std::chrono::seconds(1));
            safe_stack.pop();
        }
    });

    t1.join();
    t2.join();
}

std::mutex d_lock1;
std::mutex d_lock2;
int m_1 = 0;
int m_2 = 0;

void dead_lock_1(){
    while(true){
        std::cout << "dead_lock_1 begin " << std::endl;
        d_lock1.lock();
        m_1 = 1024;
        d_lock2.lock();
        m_2 = 2048;
        d_lock2.unlock();
        d_lock1.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::cout << "dead_lock_1 end " << std::endl;
    }
}

void dead_lock_2(){
    while(true){
        std::cout << "dead_lock_2 begin " << std::endl;
        d_lock2.lock();
        m_2 = 2048;
        d_lock1.lock();
        m_1 = 1024;
        d_lock1.unlock();
        d_lock2.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::cout << "dead_lock_2 end " << std::endl;
    }
}

void test_dead_lock() {
    std::thread t1(dead_lock_1);
    std::thread t2(dead_lock_2);
    t1.join();
    t2.join();
}

void atomic_lock_1(){
    std::cout << "lock_1 begin " << std::endl;
    d_lock1.lock();
    m_1 = 1024;
    d_lock1.unlock();
     std::cout << "lock_1 end " << std::endl;
}

void atomic_lock_2(){
    std::cout << "lock_2 begin " << std::endl;
    d_lock2.lock();
    m_2 = 2048;
    d_lock2.unlock();
     std::cout << "lock_2 end " << std::endl;
}

void safe_lock_1(){
    while(true){
        atomic_lock_1();
        atomic_lock_2();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void safe_lock_2(){
    while(true){
        atomic_lock_2();
        atomic_lock_1();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
void test_safe_lock() {
    std::thread t1(safe_lock_1);
    std::thread t2(safe_lock_2);
    t1.join();
    t2.join();
}

// 使用两个互斥量，同时加锁
class some_big_object {
public:
    some_big_object(int data):_data(data){}
    //拷贝构造
    some_big_object(const some_big_object& b2):_data(b2._data){
        _data = b2._data;
    }
    // 移动构造
    some_big_object(some_big_object&& b2):_data(std::move(b2._data)){

    }
    // 重载拷贝赋值运算符
    some_big_object& operator = (const some_big_object& b2){
        if (this == &b2){
            return *this;
        }
        _data = b2._data;
        return *this;
    }
    // 重载移动赋值运算符
    some_big_object& operator = (const some_big_object&& b2){
        _data = std::move(b2._data);
        return *this;
    }
    // 重载输出运算符
    friend std::ostream& operator << (std::ostream& os, const some_big_object& big_obj){
        os << big_obj._data;
        return os;
    }
    // 交换数据  用友元是为了让swap可以直接swap(b1,b2)这样调用，若是成员函数，那就只能b1.swap(b2)这样调用
    friend void swap(some_big_object& b1, some_big_object& b2){
        some_big_object temp = std::move(b1);
        b1 = std::move(b2);
        b2 = std::move(temp);
    }
private:
    int _data;
};

class big_object_mgr{
public:
    big_object_mgr(int data = 0):_obj(data){};
    void printinfo(){
        std::cout << "current obj data is " << _obj << std::endl;
    }
    friend void danger_swap(big_object_mgr&objm1, big_object_mgr&objm2);
    friend void safe_swap(big_object_mgr&objm1, big_object_mgr&objm2);
    friend void safe_swap_scope(big_object_mgr&objm1, big_object_mgr&objm2);
private:
    std::mutex _mtx;
    some_big_object _obj;
};

void danger_swap(big_object_mgr&objm1, big_object_mgr&objm2){
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2){
        return;
    }
    std::lock_guard<std::mutex> guard1(objm1._mtx);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> guard2(objm2._mtx);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

void safe_swap(big_object_mgr&objm1, big_object_mgr&objm2){
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2){
        return;
    }
    std::lock(objm1._mtx, objm2._mtx);
    std::lock_guard<std::mutex> guard1(objm1._mtx, std::adopt_lock); //领养锁，只负责解锁，不负责加锁
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> guard2(objm2._mtx, std::adopt_lock);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

void safe_swap_scope(big_object_mgr&objm1, big_object_mgr&objm2){
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2){
        return;
    }
    std::scoped_lock guard(objm1._mtx, objm2._mtx); // c++17
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

void test_danger_swap(){
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);
    std::thread t1(danger_swap, std::ref(objm1), std::ref(objm2));
    std::thread t2(danger_swap, std::ref(objm2), std::ref(objm1));
    t1.join();
    t2.join();
    objm1.printinfo();
    objm2.printinfo();
}

void test_safe_swap(){
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);
    std::thread t1(safe_swap, std::ref(objm1), std::ref(objm2));
    std::thread t2(safe_swap, std::ref(objm2), std::ref(objm1));
    t1.join();
    t2.join();
    objm1.printinfo();
    objm2.printinfo();
}

void test_safe_swap_scope(){
    big_object_mgr objm1(5);
    big_object_mgr objm2(100);
    std::thread t1(safe_swap_scope, std::ref(objm1), std::ref(objm2));
    std::thread t2(safe_swap_scope, std::ref(objm2), std::ref(objm1));
    t1.join();
    t2.join();
    objm1.printinfo();
    objm2.printinfo();
}

// 层级锁
class hierarchical_mutex {
public:
    explicit hierarchical_mutex(unsigned long value):_hierarchy_value(value), _previous_hierarchy_value(0){}
    hierarchical_mutex(const hierarchical_mutex&) = delete;
    hierarchical_mutex& operator=(const hierarchical_mutex&) = delete;
    void lock(){
        check_for_hierarchy_violation();
        _internal_mutex.lock();  // 实际锁定
        update_hierarchy_value();  //更新层级值
    }

    void unlock(){
        if (_this_thread_hierarchy_value != _hierarchy_value) {
            throw std::logic_error("mutex hierarchy violated");
        }
        _this_thread_hierarchy_value = _previous_hierarchy_value;  // 保存当前线程之前的层级值
        _internal_mutex.unlock();
    }
    
    bool try_lock(){
        check_for_hierarchy_violation();
        if (!_internal_mutex.try_lock()){
            return false;
        }
        update_hierarchy_value();
        return true;
    }
private:
    std::mutex _internal_mutex;
    unsigned long const _hierarchy_value;  // 当前层级值
    unsigned long _previous_hierarchy_value;  // 上一次层级值
    static thread_local unsigned long _this_thread_hierarchy_value; // 当前线程记录的层级值  

    void check_for_hierarchy_violation(){
        if (_this_thread_hierarchy_value <= _hierarchy_value){    
             throw  std::logic_error("mutex  hierarchy violated");
        }
    }

    void update_hierarchy_value(){
        _previous_hierarchy_value = _this_thread_hierarchy_value; 
        _this_thread_hierarchy_value = _hierarchy_value;
    }
};

thread_local unsigned long hierarchical_mutex::_this_thread_hierarchy_value(ULONG_MAX);  //初始化为最大值
void test_hierarchy_lock() {
    hierarchical_mutex  high_level_mutex(1000);
    hierarchical_mutex  low_level_mutex(500);
    std::thread t1([&high_level_mutex, &low_level_mutex]() {
        high_level_mutex.lock();
        low_level_mutex.lock();
        low_level_mutex.unlock();
        high_level_mutex.unlock();
        });
    std::thread t2([&high_level_mutex, &low_level_mutex]() {
        low_level_mutex.lock();
        high_level_mutex.lock();
        high_level_mutex.unlock();
        low_level_mutex.unlock();
        });
    t1.join();
    t2.join();
}
int main(){
    // test_lock();  // lock()-->unlock();   std::lock_guard<std::mutex> lock();自动加锁和解锁

    // test_threadsafe_stack();  // 数据竞争 抛出异常

    // test_dead_lock();  // 死锁

    // test_safe_lock();

    // 同时加锁
    // test_danger_swap();
    // test_safe_swap();
    // test_safe_swap_scope();

    test_hierarchy_lock(); // 层级锁
    return 0;
}