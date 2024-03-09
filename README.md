# C++ Concurrency

进程和线程的区别

## 线程基础

### 初始化线程对象

启动线程后要明确是等待线程结束`join()`，还是让其自主运行`detach()`。否则程序会终止（`std::thread`的析构函数会调用`std::terminate()`）。

> 等待线程结束，来保证可访问的数据是有效的。
>
> 只能对一个线程使用一次`join()`，当对其使用`joinable()`时，将返回false。

```cpp
void hello()
{
    std::cout << "Hello world !" << std::endl;
}
std::thread t1(hello);
t1.join();
```

```cpp
class background_task {
public:
    void operator()() { // 重载()运算符
        hello();
    }
};
// my_thread被当作函数对象的定义，其返回类型为std::thread, 参数为函数指针background_task()
// std::thread my_thread(background_task());  // 相当与声明了一个名为my_thread的函数

// 使用一组额外的括号，或使用新统一的初始化语法，可以避免其解释函数声明 （定义一个线程my_thread）
std::thread my_thread_1((background_task()));
std::thread my_thread_2{background_task()};

// lambda表达式
std::thread my_thread_3([](){
   hello();
});
my_thread_3.join();
```

### detach

线程允许采用分离的方式在后台独自运行。

当`oops`调用后，局部变量`some_local_state`可能被释放。

> 1. 通过智能指针传递参数。 (引用计数会随着赋值增加，可保证局部变量在使用期间不被释放)
> 2. 将局部变量的值作为参数传递。（需要局部变量有拷贝复制的功能，而且拷贝耗费空间和效率）
> 3. 将线程运行的方式修改为join。（可能会影响运行逻辑）

```cpp
struct func{
    int& _i;
    func(int & i): _i(i){}
    void operator()(){
        for (int i = 0; i < 3; i++){
            _i = i;
            std::cout << "_i is " << _i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};
void oops() {
    int some_locate_state = 0;
    func myfunc(some_locate_state);
    std::thread functhread(myfunc);
    // 访问局部变量。局部变量可能会随着}结束而回收或随着主线程退出而回收
    functhread.detach();
}
void use_join() {
    int some_locate_state = 0;
    func myfunc(some_locate_state);
    std::thread functhread(myfunc);
    functhread.join();
}
oops();
// 防止主线程退出过快
std::this_thread::sleep_for(std::chrono::seconds(1));
// 使用join
use_join();
```

### 捕获异常

> 捕获异常，并且在异常情况下保证子线程稳定运行结束后，主线程抛出异常结束运行。

```cpp
void catch_exception() {
    int some_locate_state = 0;
    func myfunc(some_locate_state);
    std::thread functhread(myfunc);
    try {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    } catch (std::exception& e) {
        functhread.join();
        throw;
    }
    functhread.join();
}
```

线程守卫：采用RAII技术，保证线程对象析构的时候等待线程运行结束，回收资源

```cpp
// RAII 资源获取初始化
class thread_guard {
private:
    std::thread& _t;
public:
    explicit thread_guard(std::thread& t): _t(t){}
    ~thread_guard() {
        // join只能调用一次
        if (_t.joinable()){
            _t.join();
        }
    }
    thread_guard(thread_guard const&) = delete;
    thread_guard& operator=(thread_guard const&) = delete;
};

void  auto_guard() {
    int some_locate_state = 0;
    func myfunc(some_locate_state);
    std::thread functhread(myfunc);
    thread_guard g(functhread);
    std::cout << "auto guard finished" << std::endl;
}
```
### 参数传递
```cpp
void print_str(int i, std::string const& s) {
    std::cout << "i is " << i << ", str is " << s << std::endl;
}

void danger_oops(int som_param) {
    char buffer[1024];
    sprintf(buffer, "%i", som_param);
    std::thread t(print_str, 3, buffer); // 局部变量buffer可能回收
    t.detach();
    std::cout << "danger oops finished" << std::endl;
}

void safe_oops(int som_param) {
    char buffer[1024];
    sprintf(buffer, "%i", som_param);
    std::thread t(print_str, 3, std::string(buffer)); // 显示创建一个std::string对象
    t.detach();
    std::cout << "safe oops finished" << std::endl;
}
```
当线程要调用的回调函数参数为引用类型时，需要将参数显示转化为引用对象传递给线程的构造函数。
```cpp
void chage_param(int& param){
    param++;
}

void ref_oops(int som_param) {
    std::cout << "before change, param is " << som_param << std::endl;
    std::thread t(chage_param,std::ref(som_param));// 不加stds:ref会盲目复制，传递的是副本的引用即data副本(copy)的引用
    t.join();
    std::cout << "after change, param is " << som_param << std::endl;
}
```
绑定类的成员函数，必须添加`&`
```cpp
class X {
public:
    void do_lengthy_work(){
        std::cout << "do_lengthy_work " << std::endl;
    }
};

void bind_class_oops() {
    X my_x;
    std::thread t(&X::do_lengthy_work, &my_x);
    t.join();
}
```
有时候传递给线程的参数是独占的(不支持拷贝赋值和构造)，可以通过`std::move`的方式将参数的所有权转移给线程
```cpp
void deal_unique(std::unique_ptr<int> p) {
    std::cout << "unique ptr data is " << *p << std::endl;
    (*p)++;
    std::cout << "after unique ptr data is " << *p << std::endl;
}
void move_oops() {
    auto p = std::make_unique<int>(100);
    std::thread t(deal_unique, std::move(p));
    t.join();
}
```

### 线程归属

使用`std::move`移动归属；

不能将一个线程的管理权交给一个已经绑定线程的变量，会触发线程的terminate函数引发崩溃

```cpp
void some_function(){
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
void some_other_function(){
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
std::thread t1(some_function);
std::thread t2 = std::move(t1);

t1 = std::thread(some_other_function);
std::thread t3;
t3 = std::move(t2);
// t1 = std::move(t3);  // 将一个线程的管理权交给一个已经绑定线程的变量，会触发线程的terminate函数引发崩溃
std::this_thread::sleep_for(std::chrono::seconds(10));
```

### 容器存储

生成一批线程并等待它们完成。初始化多个线程存储在vector中, 采用的时emplace方式，可以直接根据线程构造函数需要的参数构造，这样就避免了调用thread的拷贝构造函数。

```cpp
void param_function(int a) {
    std::cout << "param is " << a << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
}
void use_vector() {
    unsigned int N = std::thread::hardware_concurrency(); 
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < N; ++i) {
        threads.emplace_back(param_function, i);
    }
    for (auto& entry : threads) {
        if (entry.joinable()) {
            entry.join();
        } 
    }
    threads.clear();
}
```

### 选择运行数量

> `std::thread::hardware_concurrency()`函数，它的返回值是一个指标，表示程序在各次运行中可真正并发的线程数量。

```cpp
template<typename Iterator, typename T>
struct accumulate_block{
    void operator()(Iterator first, Iterator last, T& result){
        result = std::accumulate(first, last, result);
    }
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init){
    unsigned long const length = std::distance(first, last);
    if (!length) {  // 1.输入为空，返回初始值init
        return init;
    }
    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;  // 2.需要的线程最大数（向上取整）
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads!=0 ? hardware_threads : 2, max_threads); // 3.实际的线程选择数量
    unsigned long const block_size = length / num_threads; // 4.每个线程待处理的条目数量，步长

    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads - 1);  // 5.初始化了(num_threads - 1)个大小的vector，因为主线程也参与计算

    Iterator block_start = first;
    for (unsigned long i = 0; i < num_threads - 1; ++i){
        Iterator block_end = block_start;
        std::advance(block_end, block_size);  // 6. 递进block_size迭代器到当前块的结尾
        threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start, block_end, std::ref(results[i])); // 7.启动新的线程计算结果
        block_start = block_end;  // 8.更新起始位置
    }
    accumulate_block<Iterator, T>()(
        block_start, last, results[num_threads - 1]);     // 9. 主线程计算，处理最后的块
        for (auto& entry : threads){
            if (entry.joinable()){
                entry.join();
            }
        }
    return std::accumulate(results.begin(), results.end(), init);  // 10. 累加
}
void use_parallel_acc(int N) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector <int> vec(N);
    for (int i = 0; i < N; i++) {
        vec.push_back(i);
    }
    int sum = 0;
    sum = parallel_accumulate<std::vector<int>::iterator, int>(vec.begin(), vec.end(), sum);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeDuration = end - start;
    double duration = timeDuration.count();
    std::cout << "use_parallel_acc sum is " << sum << " duration: " << duration << std::endl;
}
```

### 识别线程

> 获取线程ID，根据线程id是否相同判断是否同一个线程
>
> * 通过`get_id()`成员函数来获取
> * `std::this_thread::get_id()`获取

```cpp
void do_subthread(){
    std::cout << "do sub thread work " << std::this_thread::get_id() << std::endl;
}

void thread_id(){
    std::thread::id master_thread = std::this_thread::get_id();
    std::thread t(do_subthread);
    std::cout << "do_subthread id: " << t.get_id() << std::endl; // 线程可能没运行，可能会返回一个空的 std::thread::id
    t.join();
    if (std::this_thread::get_id() == master_thread){
        std::cout << "do master thread work: "<<  std::this_thread::get_id() << std::endl;
    }
    std::cout << "do common thread work: " <<  std::this_thread::get_id() << std::endl;
}
```

## 锁

###  避免竞争

* 保护机制封装数据结构

  * 互斥量（mutex)  ----`lock`加锁 和`unlock`解锁

    > `std::lock_guard<std::mutex> lock(mtx1)`：互斥量`RAII`惯用法，自动加锁和解锁
    >
    > > 不要将对受保护数据的指针和引用传递到锁的范围之外
    >
    > 死锁 `deadlock`：每个线程都在等待另一个释放其`mutex`
    >
    > > 使用相同顺序锁定两个`mutex`
    > >
    > > 将加锁和解锁的功能封装为独立的函数
    > >
    > > 使用两个互斥量，同时加锁
    >
    > 层级锁：同一个函数内部加多个锁的情况，要尽可能避免循环加锁，自定义一个层级锁来保证项目中对多个互斥量加锁时是有序的。

* 修改数据结构的设计及不变量 （无锁编程）

### 同时加锁

```cpp
方法1：
std::lock(objm1._mtx, objm2._mtx);
std::lock_guard<std::mutex> guard1(objm1._mtx, std::adopt_lock); //领养锁，只负责解锁，不负责加锁
std::lock_guard<std::mutex> guard2(objm2._mtx, std::adopt_lock);
方法2：
std::scoped_lock guard(objm1._mtx, objm2._mtx); // c++17
```

### 层级锁

```cpp
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
```



# 进程

fork前是多线程，fork后是不会继续运行多线程

# 参考阅读

[C++并发编程(中文版)(C++ Concurrency In Action)](https://www.bookstack.cn/read/Cpp_Concurrency_In_Action/README.md)

[恋恋风辰官方博客 -并发编程](https://llfc.club/category?catid=225RaiVNI8pFDD5L4m807g7ZwmF#!aid/2TayNx5QxbGTaWW5s48vMjtuvCB)

> [对应B站视频](https://www.bilibili.com/video/BV1FP411x73X) -- [对应gitee](https://gitee.com/secondtonone1/boostasio-learn/tree/master/concurrent)