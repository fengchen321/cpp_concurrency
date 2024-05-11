#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <cassert>
#include <vector>

class Spinlock {
public:
    Spinlock():flag(ATOMIC_FLAG_INIT){}
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire));
    }
    void unlock() {
        flag.clear(std::memory_order_release);
    }
private:
    std::atomic_flag flag;
};

void test_Spinlock() {
    Spinlock spinlock;
    std::thread t1([&spinlock](){
        spinlock.lock();
        for (int i = 0; i < 10; ++i) {
            std::cout << "?";
        }
        std::cout << std::endl;
        spinlock.unlock();
    });

    std::thread t2([&spinlock](){
        spinlock.lock();
        for (int i = 0; i < 10; ++i) {
            std::cout << "!";
        }
        std::cout << std::endl;
        spinlock.unlock();
    });
    
    t1.join();
    t2.join();
}

std::atomic<bool> x,  y;
std::atomic<int> z;

void write_x_then_y(std::memory_order kind) {
    x.store(true, kind);
    y.store(true, kind);
}

void read_y_then_x(std::memory_order kind) {
    while(!y.load(kind)) {
        std::cout << "y load false" << std::endl;
    }
    if (x.load(kind)){
        ++z;
    }
}

void test_memory_order(std::memory_order kind) {
    std::thread t1(write_x_then_y, kind);
    std::thread t2(read_y_then_x, kind);
    t1.join();
    t2.join();
    std::cout << "Test with memory order: " << kind << " completed. z = " << z.load() << std::endl;
    assert(z.load() != 0);
}

void print_vector(std::vector<int>& v) {
    for (int& i : v) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
}
void test_order_relaxed() {
    std::atomic<int> a{0};
    std::vector<int> v3, v4;
    std::thread t1([&a](){
        for (int i = 0; i < 10; i += 2) {
            a.store(i, std::memory_order_relaxed);
        }
    });
    std::thread t2([&a](){
        for (int i = 1; i < 10; i += 2) {
            a.store(i, std::memory_order_relaxed);
        }
    });
    std::thread t3([&v3, &a](){
        for (int i = 0; i < 10; ++i) {
            v3.push_back(a.load(std::memory_order_relaxed));
        }
    });
    std::thread t4([&v4, &a](){
        for (int i = 0; i < 10; ++i) {
            v4.push_back(a.load(std::memory_order_relaxed));
        }
    });
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    print_vector(v3);
    print_vector(v4);
}

void test_acquire_release() {
    std::atomic<bool> rx, ry;
    std::thread t1([&]() {
        rx.store(true, std::memory_order_relaxed); 
        ry.store(true, std::memory_order_release); 
        });
    std::thread t2([&]() {
        while (!ry.load(std::memory_order_acquire));
        assert(rx.load(std::memory_order_relaxed));
        });
    t1.join();
    t2.join();
}


void test_order_relaxed_1() {
    std::atomic<bool> rx, ry;

	//sequenc before
	std::thread t1([&]() {
		rx.store(true, std::memory_order_relaxed); 
		ry.store(true, std::memory_order_relaxed); 
		});


	std::thread t2([&]() {
		while (!ry.load(std::memory_order_relaxed)); 
		assert(rx.load(std::memory_order_relaxed)); 
		});

	t1.join();
	t2.join();
}

void test_acquire_release_1() {
    std::atomic<bool> rx, ry;

	//sequenc before
	std::thread t1([&]() {
		rx.store(true, std::memory_order_relaxed); 
		ry.store(true, std::memory_order_release); 
		});


	std::thread t2([&]() {
		while (!ry.load(std::memory_order_acquire)); 
		assert(rx.load(std::memory_order_relaxed)); 
		});

	t1.join();
	t2.join();
}

void test_acquire_release_danger() {
	std::atomic<int> xd{0}, yd{0};
	std::atomic<int> zd;

	std::thread t1([&]() {
		xd.store(1, std::memory_order_release);
		yd.store(1, std::memory_order_release);
		});

	std::thread t2([&]() {
		yd.store(2, std::memory_order_release);
		});


	std::thread t3([&]() {
		while (!yd.load(std::memory_order_acquire));
		assert(xd.load(std::memory_order_acquire) == 1);
		});

	t1.join();
	t2.join();
	t3.join();
}

void test_release_sequence() {
	std::vector<int> data;
	std::atomic<int> flag{0};
	
	std::thread t1([&]() {
		data.push_back(42);
		flag.store(1, std::memory_order_release);
		});

	std::thread t2([&]() {
		int expected = 1;
		while (!flag.compare_exchange_strong(expected, 2, std::memory_order_relaxed))
			expected = 1;
		});

	std::thread t3([&]() {
		while (flag.load(std::memory_order_acquire) < 2);
		assert(data.at(0) == 42);
		});

	t1.join();
	t2.join();
	t3.join();
}

void test_consume_dependency() {
	std::atomic<std::string*> ptr;
	int data;

	std::thread t1([&]() {
		std::string* p = new std::string("Hello World");
		data = 42;
		ptr.store(p, std::memory_order_release);
		});

	std::thread t2([&]() {
		std::string* p2;
		while (!(p2 = ptr.load(std::memory_order_consume)));
		assert(*p2 == "Hello World");
		assert(data == 42);
		});

	t1.join();
	t2.join();
}

int main()
{
    // test_Spinlock(); 

    // test_memory_order(std::memory_order_relaxed);
    // test_memory_order(std::memory_order_seq_cst);

    // test_order_relaxed();
    // test_acquire_release();

    // test_order_relaxed_1();
    // test_acquire_release_1();

    // test_acquire_release_danger();

    // test_release_sequence();
    test_consume_dependency();
    return 0;
}
