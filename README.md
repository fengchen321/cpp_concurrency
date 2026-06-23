# C++ Concurrency

进程和线程的区别

## 线程基础

### 初始化线程对象

启动线程后要明确是等待线程结束`join()`，还是让其自主运行`detach()`。否则程序会终止(`std::thread`的析构函数会调用`std::terminate()`)。

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

// 使用一组额外的括号，或使用新统一的初始化语法，可以避免其解释函数声明 (定义一个线程my_thread)
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
> 2. 将局部变量的值作为参数传递。(需要局部变量有拷贝复制的功能，而且拷贝耗费空间和效率)
> 3. 将线程运行的方式修改为join。(可能会影响运行逻辑)

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

自动join的线程类 `joining_thread`

```cpp
class joining_thread {
    std::thread  _t;
public:
    joining_thread() noexcept = default;
    template<typename Callable, typename ...  Args>
    explicit  joining_thread(Callable&& func, Args&& ...args):
        _t(std::forward<Callable>(func),  std::forward<Args>(args)...){}
    explicit joining_thread(std::thread  t) noexcept: _t(std::move(t)){}
    joining_thread(joining_thread&& other) noexcept: _t(std::move(other._t)){}
    joining_thread& operator=(joining_thread&& other) noexcept {
        if (joinable()) {
            join();
        }
        _t = std::move(other._t);
        return *this;
    }

	joining_thread& operator=(std::thread other) noexcept {
		if (joinable()) {
			join();
		}
		_t = std::move(other);
		return *this;
	}

    ~joining_thread() noexcept {
        if (joinable()) {
            join();
        }
    }

    void swap(joining_thread& other) noexcept {
        _t.swap(other._t);
    }

    std::thread::id   get_id() const noexcept {
        return _t.get_id();
    }

    bool joinable() const noexcept {
        return _t.joinable();
    }

    void join() {
        _t.join();
    }

    void detach() {
        _t.detach();
    }

    std::thread& as_thread() noexcept {
        return _t;
    }

    const std::thread& as_thread() const noexcept {
        return _t;
    }
};
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
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;  // 2.需要的线程最大数(向上取整)
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

###  避免竞争 `lock_guard`

* 保护机制封装数据结构

  * 互斥量(mutex)  ----`lock`加锁 和`unlock`解锁

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
    > >
    > > 减少锁的使用范围
    >
    > 层级锁：同一个函数内部加多个锁的情况，要尽可能避免循环加锁，自定义一个层级锁来保证项目中对多个互斥量加锁时是有序的。

* 修改数据结构的设计及不变量 (无锁编程)

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

### `unique_lock`

`unique_lock`：可以手动解锁。，通过`unique_lock`的`owns_lock`判断是否持有锁

```cpp
std::mutex mtx;
int shared_data = 0;
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
```
支持领养和延迟加锁

* 将`std::adopt_lock`作为第二参数传入构造函数，对互斥量进行管理
* 将`std::defer_lock`作为第二参数传入构造函数，表明互斥量应保持解锁状态。

```cpp
int a = 10, b = 100;
std::mutex mtx1;
std::mutex mtx2;

void safe_swap_adopt(){
    std::lock(mtx1, mtx2);
    std::unique_lock<std::mutex> guard1(mtx1, std::adopt_lock); 
    std::unique_lock<std::mutex> guard2(mtx2, std::adopt_lock); 
    std::swap(a,b);
    guard1.unlock(); // 可自动释放， 已经领养不能mtx1.unlock()
    guard2.unlock();
    std::cout << "a = " << a << ", b = " << b << std::endl;
}

void safe_swap_defer(){
    std::unique_lock<std::mutex> guard1(mtx1, std::defer_lock); 
    std::unique_lock<std::mutex> guard2(mtx2, std::defer_lock); 
    std::lock(guard1, guard2);
    std::swap(a,b);
    std::cout << "a = " << a << ", b = " << b << std::endl;
}
```

`mutex`是不支持移动和拷贝的，`unique_lock`可移动，不可赋值

```cpp
std::unique_lock<std::mutex> get_lock() {
    std::unique_lock<std::mutex> lk(mtx);
    shared_data++;
    return lk;
}
void test_return() {
    std::unique_lock<std::mutex> lk(get_lock());
    shared_data++;
}
```

锁的粒度：表示加锁的精细程度。

一个锁的粒度要足够大，以保证可以锁住要访问的共享数据。
一个锁的粒度要足够小，以保证非共享的数据不被锁住影响性能。

```
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
```

### `shared_lock`

C++11标准没有共享互斥量，可以使用boost提供的`boost::shared_mutex`

`std::shared_mutex`(c++17)

> * 提供`lock()`、`try_lock_for()`和`try_lock_until()`用于获取互斥锁的函数
> * 提供`try_lock_shared()`和`lock_shared()`用于获取共享锁的函数
> * 当 `std::shared_mutex` 被锁定后，其他尝试获取该锁的线程将会被阻塞，直到该锁被解锁

`std::shared_timed_mutex `(c++14、17)

> * 提供`lock()`、`try_lock_for()`和`try_lock_until()`用于获取互斥锁的函数
> * 提供`try_lock_shared()`和`lock_shared()`用于获取共享锁的函数 (超时机制)
> * 尝试获取共享锁时，如果不能立即获得锁，`std::shared_timed_mutex` 会设置一个超时，超时过后如果仍然没有获取到锁，则操作将返回失败。

写操作需要独占锁。而读操作需要共享锁。

```cpp
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
```

### `recursive_lock`

出现一个接口调用另一个接口的情况，如果用普通的`std::mutex`就会出现卡死

```cpp
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
```

## 同步并发操作

### 条件变量

> `std::condition_variable `, 与`std::mutex`一起
>
> `std::condition_variable_any`，与满足最低标准的互斥量一起

条件不满足时(num 不等于1 时)`cvA.wait`就会挂起，等待线程B通知通知线程A唤醒，线程B采用的是`cvA.notifyone`

```cpp
void ResonableImplemention() {
    std::thread t1([](){
        while(true){
            std::unique_lock<std::mutex> lk(mtx_num);
            // 方法一
            // while (num != 1) {
            //     cvA.wait(lk);
            // }
            // 方法二
            cvA.wait(lk, []() {
                return num == 1;
            });
            std::cout << "thread A print 1....." << std::endl;
            num++;
            cvB.notify_one();
            }
    });

    std::thread t2([](){
        while(true){
            std::unique_lock<std::mutex> lk(mtx_num);
            cvB.wait(lk, []() {
                return num == 2;
            });
            std::cout << "thread B print 2....." << std::endl;
            num--;
            cvA.notify_one();
            }
    });
    t1.join();
	t2.join();
}
```

```cpp
// 队列实现，和之前栈实现类似
template<typename T>
class threadsafe_queue
{
public:
    threadsafe_queue(){}
    threadsafe_queue(const threadsafe_queue& other) {
        std::lock_guard<std::mutex> lk(other.mut);
        data_queue = other.data_queue;
    }
    threadsafe_queue& operator=(const threadsafe_queue&) = delete;

    void push(T new_value) {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(new_value);
        data_cond.notify_one();
    }
    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]{return !data_queue.empty();});
        value = data_queue.front();
        data_queue.pop();
    }
    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]{return !data_queue.empty();});
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty()) {
            return false;
        }
        value = data_queue.front();
        data_queue.pop();
        return true;
    }
    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }
    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
};
```

### async

> 用于异步执行函数的模板函数，它返回一个 `std::future` 对象，该对象用于获取函数的返回值。
>
> 类似`std::thread`,通过添加额外的调用参数，向函数传递额外的参数。

```cpp
std::string fetchDataFromDB(std::string query) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return "Data: " + query;
}
void use_asyc() {
    // 使用 std::async 异步调用 fetchDataFromDB
    std::future<std::string> resultFromDB = std::async(std::launch::async, fetchDataFromDB, "Data");

    // 在主线程中做其他事情
	std::cout << "Doing something else..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(4));
    std::cout << "past 4s" << std::endl;

	// 从 future 对象中获取数据
	std::string dbData = resultFromDB.get();
	std::cout << dbData << std::endl;
}
```

`std::async` 创建了一个新的线程(或从内部线程池中挑选一个线程)并自动与一个 `std::promise` 对象相关联。`std::promise` 对象被传递给 `fetchDataFromDB` 函数，函数的返回值被存储在 `std::future` 对象中。在主线程中，使用 `std::future::get` 方法从 `std::future` 对象中获取数据。注意，在使用 `std::async` 的情况下，必须使用 `std::launch::async` 标志来明确表明希望函数异步执行。

**启动策略**：在`std::launch`枚举中定义。

```cpp
  enum class launch
  {
    async = 1,
    deferred = 2
  };
```

* `std::launch::async`：表明函数必须在其所在的独立线程上执行
* `std::launch::deferred`：表明函数调用被延迟到`std::future::get()`或`std::future::wait()`时才执行。(要结果的时候才执行)
* `std::launch::async | std::launch::deferred`：(默认使用)任务可以在一个单独的线程上异步执行，也可以延迟执行，具体取决于实现。

### future 期望值

> 唯一期望值：`std::futurte<>`；只能与一个指定事件相关联
>
> 共享期望值：`std::shared_future<>`：可关联多个事件，所有实例同时变为就绪状态。

`std::future::get()`：阻塞调用，用于获取并返回任务的结果；只能调用一次

`std::future::wait()`： 阻塞调用，只是等待任务完成；可以被多次调用

`std::future::wait_for()`和`std::future::wait_until`检查异步操作是否已完成，返回一个表示操作状态的`std::future_status`值

**任务与future关联**

`std::packaged_task`：是一个可调用对象，它包装了一个任务，该任务可以在另一个线程上运行。它可以捕获任务的返回值或异常，并将其存储在`std::future`对象中，以便以后使用。

> 1. 创建一个`std::packaged_task`对象，该对象包装了要执行的任务。
> 2. 调用`std::packaged_task`对象的`get_future()`方法，该方法返回一个与任务关联的`std::future`对象。
> 3. 在另一个线程上调用`std::packaged_task`对象的`operator()`，以执行任务。
> 4. 在需要任务结果的地方，调用与任务关联的`std::future`对象的`get()`方法，以获取任务的返回值或异常。

```cpp
int my_task() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "my task run 5 s" << std::endl;
    return 0;
}

void use_package() {
    std::packaged_task<int()> task(my_task);     //创建一个`std::packaged_task`对象，该对象包装了要执行的任务。
    std::future<int> result = task.get_future(); //  // 获取与任务关联的 std::future 对象  
    std::thread t(std::move(task));  // 在另一个线程上执行任务  
    t.detach();
    int value = result.get();      // 等待任务完成并获取结果  
    std::cout << "The result is: " << value << std::endl;
}
```

**共享类型的future**

多个线程等待同一个异步操作的结果

```cpp
void myFunction(std::promise<int>&& promise) {
	std::this_thread::sleep_for(std::chrono::seconds(1));
	promise.set_value(42); // 设置 promise 的值
}

void threadFunction(std::shared_future<int> future) {
	try {
		int result = future.get();
		std::cout << "Result: " << result << std::endl;
	}
	catch (const std::future_error& e) {
		std::cout << "Future error: " << e.what() << std::endl;
	}
}

void use_shared_future() {
	std::promise<int> promise;
	std::shared_future<int> future = promise.get_future();
	std::thread myThread1(myFunction, std::move(promise)); // 将 promise 移动到线程中
	// 使用 share() 方法获取新的 shared_future 对象  
	std::thread myThread2(threadFunction, future);
	std::thread myThread3(threadFunction, future);
	myThread1.join();
	myThread2.join();
	myThread3.join();
}
```

**异常处理**

```cpp
void may_throw()
{
	throw std::runtime_error("Oops, something went wrong!"); // 抛出一个异常
}

void use_future_exception() {
	std::future<void> result(std::async(std::launch::async, may_throw)); // 创建一个异步任务
	try {
		result.get(); // 获取结果(如果在获取结果时发生了异常，那么会重新抛出这个异常)
	}
	catch (const std::exception& e) {
		std::cerr << "Caught exception: " << e.what() << std::endl; // 捕获并打印异常
	}
}
```

### promise 承诺值

> `std::promise`用于在某一线程中**设置**某个值或异常，
>
> > `std::promise::set_value()`：设置异步操作的结果值
> >
> > `std::promise::set_exception`：设置异常情况
> >
> > > 接受一个`std::exception_ptr`参数，该参数可以通过调用`std::current_exception()`方法获取
>
> `std::future`则用于在另一线程中**获取**这个值或异常。

```cpp
void set_value(std::promise<int> prom) {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    prom.set_value(10);
    std::cout << "promise set value success" << std::endl;
}

void use_promise_setvalue() {
    std::promise<int> prom;   // 创建一个 promise 对象
    std::future<int> fut = prom.get_future(); // 获取与 promise 相关联的 future 对象
    std::thread t(set_value, std::move(prom)); // 在新线程中设置 promise 的值
    std::cout << "Waiting for the thread to set the value...\n";
	std::cout << "Value set by the thread: " << fut.get() << '\n'; // 在主线程中获取 future 的值
	t.join();
}
// 随着局部作用域}的结束，prom可能被释放也可能会被延迟释放，如果立即释放则fut.get()获取的值会报error_value的错误
void bad_promise_setvalue() {
    std::thread t;
    std::future<int> fut;
    {
        std::promise<int> prom;   // 创建一个 promise 对象
        fut = prom.get_future(); // 获取与 promise 相关联的 future 对象
        t = std::thread(set_value, std::move(prom)); // 在新线程中设置 promise 的值
    }
    std::cout << "Waiting for the thread to set the value...\n";
	std::cout << "Value set by the thread: " << fut.get() << '\n'; // 在主线程中获取 future 的值
	t.join();
}

void set_exception(std::promise<void> prom) {
    try {
        throw std::runtime_error("An error occurred!");
    }
    catch (...) {
        prom.set_exception(std::current_exception());
    }
}
// 注：子线程调用了set_exception，主线程一定要捕获这个异常，否则崩溃
void use_promise_setexception() {
    std::promise<void> prom;   // 创建一个 promise 对象
    std::future<void> fut = prom.get_future(); // 获取与 promise 相关联的 future 对象
    std::thread t(set_exception, std::move(prom)); // 在新线程中设置 promise 的异常
    try {
		std::cout << "Waiting for the thread to set the exception...\n";
		fut.get();
	}
	catch (const std::exception& e) {
		std::cout << "Exception set by the thread: " << e.what() << '\n';
	}
	t.join();
}
```

### 快速排序实例

开辟一个一次性的线程执行并行任务，主线程可以通过future在合适的时机执行等待汇总结果。

```cpp
template<typename T>
void quick_sort_recursive(T q[], int l, int r){
    if (l >= r) return;
    T x = q[(l + r + 1) >> 1];
    int i = l - 1, j = r + 1;
    while(i < j){
        do i++; while(q[i] < x);
        do j--; while(q[j] > x);
        if (i < j) std::swap (q[i], q[j]);
    }
    quick_sort_recursive(q, l, i - 1);
    quick_sort_recursive(q, i, r);
}

template<typename T>
void quick_sort(T q[], int len) {
    quick_sort_recursive(q, 0, len - 1);
}
```

串行版本

```cpp
template<typename T>
std::list<T> sequential_quick_sort(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin()); // 将 input 列表中的第一个元素移动到 result 列表的起始位置，并且在 input 列表中删除该元素
    T const& pivot = *result.begin(); // 取首元素作为 x
    // partition 分区函数，使得满足条件的元素排在不满足条件元素之前。divide_point指向的是input中第一个大于等于pivot的地址
    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t){return t < pivot;});
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point); // 小于pivot的元素放在lower_part里
    auto new_lower(sequential_quick_sort(std::move(lower_part)));
    auto new_higher(sequential_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower);
    return result;
}
```

并行版本

```cpp
template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin(); 

    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t){return t < pivot;});
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);
    std::future<std::list<T>> new_lower(std::async(parallel_quick_sort<T>, std::move(lower_part)));
    auto new_higher(parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}
```

### 并发设计模式

#### Actor 参与者模式

> 系统由多个独立的并发执行的actor组成。每个actor都有自己的状态、行为和邮箱(用于接收消息)。Actor之间通过消息传递进行通信，而不是共享状态。

#### CSP(Communicating Sequential Processes)通信顺序进程

> 各个进程之间彼此独立，通过发送和接收消息进行通信，通道用于确保进程之间的同步。

生产者消费者模型

```cpp
template<typename T>
class Channel {
    public:
      Channel(size_t capacity = 0):capacity_(capacity){}

      bool send(T value) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_producer_.wait(lock, [this]() {return (capacity_ == 0 && queue_.empty()) || queue_.size() < capacity_ || closed_;});
        if (closed_) {
            return false;
        }
        queue_.push(value);
        cv_consumer_.notify_one();
        return true;
      }

      bool receive(T& value) {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_consumer_.wait(lock, [this]() {return !queue_.empty() ||  closed_;});
        if (closed_ && queue_.empty()) {
            return false;
        }
        value = queue_.front();
        queue_.pop();
        cv_producer_.notify_one();
        return true;
      }
      void close() {
        std::unique_lock<std::mutex> lock(mtx_);
        closed_ = true;
        cv_producer_.notify_all();
        cv_consumer_.notify_all();
      }
    private:
      std::queue<T> queue_;
      std::mutex mtx_;
      std::condition_variable cv_producer_;
      std::condition_variable cv_consumer_;
      size_t capacity_;
      bool closed_ = false;
};
```

ATM实例

> handle成员函数：当函数返回一个类类型的局部变量时会先调用移动构造，如果没有移动构造再调用拷贝构造。

## 内存模型和原子类型

原子操作

> 无法拷贝构造，拷贝赋值

| 操作方式                          | 可选顺序                                                     |
| --------------------------------- | ------------------------------------------------------------ |
| `store`操作 存储操作              | `memory_order_relaxed`,`memory_order_release`,`memory_order_seq_cst` |
| `Load`操作 载入操作               | `memory_order_relaxed`,`memory_order_consume`,`memory_order_acquire`,`memory_order_seq_cst` |
| `read-modify-write`(读-改-写)操作 | `memory_order_relaxed`,`memory_order_consume`,`memory_order_acquire`,<br />`memory_order_release`,`memory_order_acq_rel`, `memory_order_seq_cst` |

| 成员函数                                                     |                             说明                             |
| ------------------------------------------------------------ | :----------------------------------------------------------: |
| `void store(T desired, std::memory_order order = std::memory_order_seq_cst)` |                       写入(释放操作)                        |
| `T load(std::memory_order order = std::memory_order_seq_cst )` |                       读取(获取操作)                       |
| `bool compare_exchange_weak(T& expected, T desired, std::memory_order order =std::memory_order_seq_cst)`<br />当前值与期望值(expect)相等时，修改当前值为设定值(desired)，返回true；<br />当前值与期望值(expect)不等时，将期望值(expect)修改为当前值，返回false； |    读改写：比较-交换操作；可能保存失败，往往配合循环使用     |
| `bool compare_exchange_strong(T& expected, T desired, std::memory_order order =std::memory_order_seq_cst)` | 读改写：内部含循环，保存的值需要耗时计算(或体积较大的原子类型)选择其更合理 |
| `T exchange(T desired, std::memory_order order = std::memory_order_seq_cst)` |                            读改写                            |

内存顺序

| 内存序                 | 说明                                                         |
| ---------------------- | ------------------------------------------------------------ |
| `memory_order_relaxed` | 松散内存序，只用来保证对原子对象的操作是原子的，对顺序不做保证(允许指令重排) |
| `memory_order_consume` | 适用读操作，阻止对这个原子量有依赖的操作重排到前面去(限制读操作之后的部分操作，不允许指令重排) |
| `memory_order_acquire` | 适用读操作，在读取某原子对象时，当前线程的任何后面的读写操作都不允许重排到这个操作的前面去(读操作之后的部分，不允许指令重排) |
| `memory_order_release` | 适用写操作，在写入某原子对象时，当前线程的任何前面的读写操作都不允许重排到这个操作的后面去(写操作之前的部分，不允许指令重排) |
| `memory_order_acq_rel` | 适用读写操作,一个读-修改-写操作同时具有获得语义和释放语义，即它前后的任何读写操作都不允许重排(读写操作不允许指令重排) |
| `memory_order_seq_cst` | 顺序一致性语义,对于读操作相当于获取，对于写操作相当于释放，对于读-修改-写操作相当于获得释放，是所有原子操作的默认内存序(不允许指令重排) |

自旋锁：当一个线程尝试获取锁时，如果锁已经被其他线程持有，那么该线程就会不断地循环检查锁的状态，直到成功获取到锁为止。

```cpp
class Spinlock {
public:
    Spinlock():flag(ATOMIC_FLAG_INIT){}
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire));// 获取旧值并设置标志
    }
    void unlock() {
        flag.clear(std::memory_order_release); // clear为存储操作，显示采用释放语义将标志清零
    }
private:
    std::atomic_flag flag;
};
```

## 环形队列

```cpp
template<typename T, size_t Cap>
class CircularQueLk:private std::allocator<T> {
public:
    CircularQueLk()
        :_max_size(Cap + 1),
         _data(std::allocator<T>::allocate(_max_size)), 
         _head(0), 
         _tail(0)
         {}
    CircularQueLk(const CircularQueLk&) = delete;
    CircularQueLk& operator=(const CircularQueLk&) volatile = delete; // 为什么拷贝复制有两个
    CircularQueLk& operator=(const CircularQueLk&) = delete;
    ~CircularQueLk() {
        std::lock_guard<std::mutex> lock(_mtx);
        while(_head != _tail) {
            std::allocator<T>::destroy(_data + _head);
            _head = (_head + 1) % _max_size;
        }
        std::allocator<T>::deallocate(_data, _max_size);
    }

    template<typename ...Args>
    bool emplace(Args&& ...args) {
        std::lock_guard<std::mutex> lock(_mtx);
        if ((_tail + 1) % _max_size == _head) {
            std::cout << "circular que full !\n";
            return false;
        }
        // 尾部位置构造一个对象
        std::allocator<T>::construct(_data + _tail, std::forward<Args>(args)...);
        _tail = (_tail + 1) % _max_size;
        return true;
    }
    // 接受左值引用版本(加const：让其接受const类型也可以接受非const类型)
    bool push(const T& val) {
        std::cout << "called push const T& version\n";
        return emplace(val);
    }

    bool push(T&& val) {
        std::cout << "called push const T&& version\n";
        return emplace(std::move(val));
    }

    bool pop(T& val) {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_head == _tail) {
            std::cout << "circular que empty !\n";
            return false;
        }
        val = std::move(_data[_head]);
        _head = (_head + 1) % _max_size;
        return true;
    }

private:
    size_t _max_size;
    T* _data;
    std::mutex _mtx;
    size_t _head = 0;
    size_t _tail = 0;
};
```

## 并发安全容器

### 线程安全队列

#### 版本一：`threadsafe_queue<T>` — 单锁粗粒度

最简单的方案，一把互斥量保护整个`std::queue`：

* `push`：加锁 → 入队 → `notify_one`唤醒等待的消费者
* `wait_and_pop`：`unique_lock` + `cv.wait`阻塞等待队列非空，然后取出队首
* `try_pop`：非阻塞，空队列直接返回false
* 提供两个重载：引用出参版和`shared_ptr`返回版

**缺点**：所有操作（push和pop）竞争同一把锁，高并发下瓶颈明显。

#### 版本二：`threadsafe_queue_ptr<T>` — 存储`shared_ptr`

队列内部存`shared_ptr<T>`而非`T`：

* `push`先在锁外构造`shared_ptr`，再加锁入队 — 减少持锁时间
* `wait_and_pop`返回`shared_ptr`时直接返回队列中的指针，避免额外拷贝/移动

**好处**：`make_shared`的内存分配在锁外完成，锁内只做指针赋值，降低了锁的粒度。

#### 版本三：`threadsafe_queue_ht<T>` — 双锁分离头尾

核心设计：用链表替换`std::queue`，**head和tail各一把锁**，push和pop可以真正并发：

```
head → [dummy] → [node1] → [node2] → ... ← tail
       (pop端)                            (push端)
```

* **head_mutex**：保护`head`指针，pop操作用
* **tail_mutex**：保护`tail`指针，push操作用
* **dummy node**：哨兵节点，`head.get() == tail`表示队列为空，避免head和tail指向同一节点时需要同时持两把锁
* **`wait_for_data`**：等待时先锁head，再通过`get_tail()`短暂锁tail判空，确认非空后返回`unique_lock`，后续`pop_head`直接复用

**关键点**：push只锁tail，pop只锁head，两者互不阻塞，实现了真正的并发访问。

## 无锁

实现无锁数据结构原则
1. 在原型设计中使用std::memory_order_seq_cst次序。正常工作后才考虑使用更宽松的次序。
2. 使用无锁的内存回收方案
   > 延时删除：将要删除的节点放入待删除列表中。暂缓全部删除对象的动作，等到没有线程访问数据结构的时候，才删除待销毁的对象
   >
   >hazard pointer 风险指针: 将节点存放到候删链表上。采用风险指针，以辨识特定对象是否正在被某线程访问
   >
   >引用计数：对各节点分别维护一个计数器。只要外部环境仍正在指涉目标对象，它就不会被删除
3. 防范ABA问题。 (ABA: 某线程读取到的值A在操作过程中被其他线程修改为值B，然后又改回值A，导致CAS操作错误地认为值未改变。)
4. 找出忙等循环，协助其他线程


# 进程

fork前是多线程，fork后是不会继续运行多线程

# 参考阅读

[C++并发编程(中文版)(C++ Concurrency In Action)](https://www.bookstack.cn/read/Cpp_Concurrency_In_Action/README.md)

[恋恋风辰官方博客 -并发编程](https://llfc.club/category?catid=225RaiVNI8pFDD5L4m807g7ZwmF#!aid/2TayNx5QxbGTaWW5s48vMjtuvCB)

> [对应B站视频](https://www.bilibili.com/video/BV1FP411x73X) -- [对应gitee](https://gitee.com/secondtonone1/boostasio-learn/tree/master/concurrent)