#include "thread_pool.h"
/*
线程池注意事项
1. 任务要是并发且无序的
2. 互斥性大，强关联不能用
*/
int main() {
    /*单例 pool实例*/ 
    auto &m1 = ThreadPool::instance();
	auto &j = ThreadPool::instance();
	std::cout << "m is " << &m1 << std::endl;
	std::cout << "j is " << &j << std::endl;

    /* 直接传递左值m，args被推断int, 但是bind时构造时通过decay_t去掉了引用变成副本右值*/
	int m = 0;
	ThreadPool::instance().commit([](int& m) {
		m = 1024;
		std::cout << "inner set m is " << m << std::endl;
        std::cout << "m address is " << &m << std::endl;
		}, m);

	std::this_thread::sleep_for(std::chrono::seconds(3));
	std::cout << "m is " << m << std::endl;
    std::cout << "m address is " << &m << std::endl;

    int n = 0;
	std::this_thread::sleep_for(std::chrono::seconds(3));
	ThreadPool::instance().commit([](int& n) {
		n = 1024;
		std::cout << "inner set n is " << n << std::endl;
        std::cout << "m address is " << &n << std::endl;
		}, std::ref(n));  // wrapper类型存的m地址 &m， 转换运算符operator _Tp&() const noexcept 返回数据地址return *_M_data
	std::this_thread::sleep_for(std::chrono::seconds(3));
	std::cout << "n is " << n << std::endl;
    std::cout << "n address is " << &n << std::endl;
    return 0;
}