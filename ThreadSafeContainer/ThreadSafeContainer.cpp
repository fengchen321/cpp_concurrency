#include <iostream>
#include <thread>
#include "ThreadSafeStack.h"
#include "ThreadSafeQueue.h"
#include "MyClass.h"

std::mutex mtx_cout;
void PrintMyClass(std::string consumer, std::shared_ptr<MyClass> data)
{
	std::lock_guard<std::mutex> lock(mtx_cout);
	std::cout << consumer << " pop data success , data is " << (*data) << std::endl;
}

void TestThreadSafeStack_Wait()
{
	threadsafe_stack_waitable<MyClass> stack;
	
	std::thread consumer1(
		[&]()
		{
			for(;;)
			{
				std::shared_ptr<MyClass> data = stack.wait_and_pop();
				PrintMyClass("consumer1", data);
			}
		}
	);

	std::thread consumer2([&]()
		{
			for (;;)
			{
				std::shared_ptr<MyClass> data = stack.wait_and_pop();
				PrintMyClass("consumer2", data);
			}
		});

	std::thread producer([&]()
		{
			for(int i = 0 ; i < 100; i++)
			{
				MyClass mc(i);
				stack.push(std::move(mc));
			}
		});

	consumer1.join();
	consumer2.join();
	producer.join();
}

void TestThreadSafeQue()
{
	threadsafe_queue<MyClass> safe_que;
	std::thread consumer1(
		[&]()
		{
			for (;;)
			{
				std::shared_ptr<MyClass> data = safe_que.wait_and_pop();
				PrintMyClass("consumer1", data);
			}
		}
	);

	std::thread consumer2([&]()
		{
			for (;;)
			{
				std::shared_ptr<MyClass> data = safe_que.wait_and_pop();
				PrintMyClass("consumer2", data);
			}
		});

	std::thread producer([&]()
		{
			for (int i = 0; i < 100; i++)
			{
				MyClass mc(i);
				safe_que.push(std::move(mc));
			}
		});

	consumer1.join();
	consumer2.join();
	producer.join();
}

void TestThreadSafeQuePtr()
{
	threadsafe_queue_ptr<MyClass> safe_que;
	std::thread consumer1(
		[&]()
		{
			for (;;)
			{
				std::shared_ptr<MyClass> data = safe_que.wait_and_pop();
				PrintMyClass("consumer1", data);
			}
		}
	);

	std::thread consumer2([&]()
		{
			for (;;)
			{
				std::shared_ptr<MyClass> data = safe_que.wait_and_pop();
				PrintMyClass("consumer2", data);
			}
		});

	std::thread producer([&]()
		{
			for (int i = 0; i < 100; i++)
			{
				MyClass mc(i);
				safe_que.push(std::move(mc));
			}
		});

	consumer1.join();
	consumer2.join();
	producer.join();
}

void TestThreadSafeQueHt()
{
	threadsafe_queue_ht<MyClass> safe_que;
	std::thread consumer1(
		[&]()
		{
			for (;;)
			{
				std::shared_ptr<MyClass> data = safe_que.wait_and_pop();
				PrintMyClass("consumer1", data);
			}
		}
	);

	std::thread consumer2([&]()
		{
			for (;;)
			{
				std::shared_ptr<MyClass> data = safe_que.wait_and_pop();
				PrintMyClass("consumer2", data);
			}
		});

	std::thread producer([&]()
		{
			for (int i = 0; i < 100; i++)
			{
				MyClass mc(i);
				safe_que.push(std::move(mc));
			}
		});

	consumer1.join();
	consumer2.join();
	producer.join();
}

int main() {
    // TestThreadSafeStack_Wait();
    // TestThreadSafeQue();
    // TestThreadSafeQuePtr();
    TestThreadSafeQueHt();
    return 0;
}