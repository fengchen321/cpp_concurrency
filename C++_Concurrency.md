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
    std::thread t(print_str, 3, buffer); // buffer可能回收
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
    std::thread t(chage_param,std::ref(som_param));
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

# 进程

fork前是多线程，fork后是不会继续运行多线程

# 参考阅读

[C++并发编程(中文版)(C++ Concurrency In Action)](https://www.bookstack.cn/read/Cpp_Concurrency_In_Action/README.md)

[恋恋风辰官方博客 -并发编程](https://llfc.club/category?catid=225RaiVNI8pFDD5L4m807g7ZwmF#!aid/2TayNx5QxbGTaWW5s48vMjtuvCB)

> [对应B站视频](https://www.bilibili.com/video/BV1FP411x73X) -- [对应gitee](https://gitee.com/secondtonone1/boostasio-learn/tree/master/concurrent)