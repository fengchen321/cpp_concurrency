#include <iostream>
#include <future>
#include <string>
#include <thread>

// 模拟一个异步任务，从数据库中获取数据
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

void use_shared_future_error() {
	std::promise<int> promise;
	std::shared_future<int> future = promise.get_future();
	std::thread myThread1(myFunction, std::move(promise)); // 将 promise 移动到线程中
	std::thread myThread2(threadFunction, std::move(future));
	std::thread myThread3(threadFunction, std::move(future)); // 不能多次move
	myThread1.join();
	myThread2.join();
	myThread3.join();
}

void may_throw()
{
	throw std::runtime_error("Oops, something went wrong!"); // 抛出一个异常
}

void use_future_exception() {
	std::future<void> result(std::async(std::launch::async, may_throw)); // 创建一个异步任务
	try {
		result.get(); // 获取结果（如果在获取结果时发生了异常，那么会重新抛出这个异常）
	}
	catch (const std::exception& e) {
		std::cerr << "Caught exception: " << e.what() << std::endl; // 捕获并打印异常
	}
}

int main() {
    // use_asyc();
    // use_package();
    // use_promise_setvalue();
    // bad_promise_setvalue();
    // use_promise_setexception();
    // use_shared_future();
    // use_shared_future_error();
    use_future_exception();
    return 0;
}