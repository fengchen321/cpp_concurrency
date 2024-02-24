#include <iostream>
#include <thread>


void hello() {
    std::cout << "Hello world !" << std::endl;
}

class background_task {
public:
    void operator()() { // 重载()运算符
        hello();
    }
};

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
    std::thread t(print_str, 3, std::string(buffer));
    t.detach();
    std::cout << "safe oops finished" << std::endl;
}

void chage_param(int& param){
    param++;
}

void ref_oops(int som_param) {
    std::cout << "before change, param is " << som_param << std::endl;
    std::thread t(chage_param,std::ref(som_param));
    t.join();
    std::cout << "after change, param is " << som_param << std::endl;
}

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

int main()
{
    // 1. 通过()初始化并启动一个线程
    std::thread t1(hello);
    // 2. 主线程等待子线程退出
    t1.join();

    // 3. my_thread被当作函数对象的定义，其返回类型为std::thread, 参数为函数指针background_task()
    // std::thread my_thread(background_task());  // 相当与声明了一个名为my_thread的函数
    // my_thread.join();
    // 使用一组额外的括号，或使用新统一的初始化语法，可以避免其解释函数声明 （定义一个线程my_thread）
    std::thread my_thread_1((background_task()));
    std::thread my_thread_2{background_task()};
    my_thread_1.join();
    my_thread_2.join();
    // 4. lambda表达式
    std::thread my_thread_3([](){
        hello();
    });
    my_thread_3.join();


    // 5. detach注意事项， 分离线程
    oops();
    // 防止主线程退出过快
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 6. 使用join
    use_join();

    // 7. 捕获异常
    catch_exception();

    // 8. 线程守卫
    auto_guard();

    // 可能崩溃
    danger_oops(100);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    safe_oops(100);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    // 绑定引用
    ref_oops(100);

    // 绑定类的成员函数
    bind_class_oops();

    // 转移
    move_oops();
    return 0;
}
