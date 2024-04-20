#include <iostream>
#include <thread>
#include <memory>
#include <functional>
#include <future>

class ReturnTest {
public:
    ReturnTest(){}
    ReturnTest(const ReturnTest&){ std::cout << "ReturnTest Copy " << std::endl;}
    // ReturnTest(ReturnTest&&){ std::cout << "ReturnTest Move " << std::endl;}
    ~ReturnTest() {std::cout << "~ReturnTest" << std::endl;}
};

ReturnTest TestCp() {
	ReturnTest tp;
	return tp;
}

// 返回值优化（Return Value Optimization, RVO） 只打印~ReturnTest
// g++ -fno-elide-constructors qa_test.cpp -o qa_test 禁用优化级别，ReturnTest Move ~ReturnTest ~ReturnTest
void return_local_test() {
    TestCp();
}

std::unique_ptr<int> ReturnUniquePtr() {
	std::unique_ptr<int>  uq_ptr = std::make_unique<int>(100);
	return  uq_ptr;
}

void return_uniqueptr() {
    auto rt_ptr = ReturnUniquePtr();
    std::cout << "rt_ptr value is " << *rt_ptr << std::endl;
}

std::thread ReturnThread() {
	std::thread t([]() {
		int i = 0;
		while (true) {
			std::cout << "i is " << i << std::endl;
			i++;
			if (i == 5) {
				break;
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		});

	return t;
}

void return_thread() {
    std::thread rt_thread = ReturnThread();
    rt_thread.join();
}
// future析构要保证其关联的任务完成，所以需要等待任务完成future才被析构
void BlockAsync() {
	std::cout << "begin block async" << std::endl;
	{
		std::future<void> futures = std ::async(std::launch::async, []() {
			std::this_thread::sleep_for(std::chrono::seconds(3));
			std::cout << "std::async called " << std::endl;
			});
	}
	std::cout << "end block async" << std::endl;
}

void DeadLock() {
	std::mutex  mtx;
	std::cout << "DeadLock begin " << std::endl;
	std::lock_guard<std::mutex>  dklock(mtx);
	{
		std::future<void> futures = std::async(std::launch::async, [&mtx]() {
			std::cout << "std::async called " << std::endl;
			std::lock_guard<std::mutex>  dklock(mtx);
			std::cout << "async working...." << std::endl;
			});
	}

	std::cout << "DeadLock end " << std::endl;
}

void DeadLock_change() {
	std::mutex  mtx;
	std::cout << "DeadLock begin " << std::endl;
    std::future<void> futures;
	std::lock_guard<std::mutex>  dklock(mtx);
	{
		futures = std::async(std::launch::async, [&mtx]() {
			std::cout << "std::async called " << std::endl;
			std::lock_guard<std::mutex>  dklock(mtx);
			std::cout << "async working...." << std::endl;
			});
	}

	std::cout << "DeadLock end " << std::endl;
}
/*
func1 中要异步执行asyncFunc函数。
func2中先收集asyncFunc函数运行的结果，只有结果正确才执行
func1启动异步任务后继续执行，执行完直接退出不用等到asyncFunc运行完
*/
int asyncFunc() {
	std::this_thread::sleep_for(std::chrono::seconds(3));
	std::cout << "this is asyncFunc" << std::endl;
	return 0;
}

void func1(std::future<int>& future_ref) {
	std::cout << "this is func1" << std::endl;
	future_ref = std::async(std::launch::async, asyncFunc);
}

void func2(std::future<int>& future_ref) {
	std::cout << "this is func2" << std::endl;
	auto future_res = future_ref.get();
	if (future_res == 0) {
		std::cout << "get asyncFunc result success !" << std::endl;
	}
	else {
		std::cout << "get asyncFunc result failed !" << std::endl;
		return;
	}
}
void first_method() {
	std::future<int> future_tmp;
	func1(future_tmp);
	func2(future_tmp);
}

template<typename Func, typename... Args  >
auto  ParallenExe(Func&& func, Args && ... args) -> std::future<decltype(func(args...))> {
	typedef    decltype(func(args...))  RetType;
	std::function<RetType()>  bind_func = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
	std::packaged_task<RetType()> task(bind_func);
	auto rt_future = task.get_future();
	std::thread t(std::move(task));

	t.detach();

	return rt_future;
}

void TestParallen1() {
	int i = 0;
	std::cout << "Begin TestParallen1 ..." << std::endl;
	{
		ParallenExe([](int i) {
			while (i < 3) {
				i++;
				std::cout << "ParllenExe thread func " << i << " times" << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			}, i);
	}
	std::cout << "End TestParallen1 ..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(4));
}

void TestParallen2() {
	int i = 0;
	std::cout << "Begin TestParallen2 ..." << std::endl;
	
	auto rt_future = ParallenExe([](int i) {
			while (i < 3) {
				i++;
				std::cout << "ParllenExe thread func " << i << " times" << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			}, i);
	
	std::cout << "End TestParallen2 ..." << std::endl;
	rt_future.wait();
}

int main() {
    // return_local_test();
    // return_uniqueptr();
    // return_thread();
    // BlockAsync();
    // DeadLock();
    // DeadLock_change();
    // first_method();
    // TestParallen1();
    TestParallen2();
    return 0;
}