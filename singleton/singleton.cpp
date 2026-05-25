#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

// c++11后是线程安全的
class SingletonStatic {
private:
    SingletonStatic(){}
    SingletonStatic(const SingletonStatic&) = delete;
    SingletonStatic& operator=(const SingletonStatic&) = delete;
public:
    static SingletonStatic& GetInstance() {
        static SingletonStatic singleton;
        return singleton;
    }
};

void print_singleton() {
    SingletonStatic& instance = SingletonStatic::GetInstance();
    std::cout << "Instance address: " << &instance << std::endl;
}

void test_singleton() {
    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(print_singleton);
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

// 饿汉式
class Singlehungry {
private:
    Singlehungry(){}
    Singlehungry(const Singlehungry&) = delete;
    Singlehungry& operator=(const Singlehungry&) = delete;
public:
    static Singlehungry* GetInstance() {
        if (singleton == nullptr) {
            singleton = new Singlehungry();
        }
        return singleton;
    }
private:
    static Singlehungry* singleton;
};
// 饿汉式初始化
Singlehungry* Singlehungry::singleton = Singlehungry::GetInstance();
void test_singletonhungry() {
    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
         threads.emplace_back([](){
            Singlehungry* instance = Singlehungry::GetInstance();
            std::cout << "Instance address: " << instance << std::endl;
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

// 懒汉式
class SingleLazy {
private:
    SingleLazy(){}
    SingleLazy(const SingleLazy&) = delete;
    SingleLazy& operator=(const Singlehungry&) = delete;
public:
    static SingleLazy* GetInstance() {
        if(singleton != nullptr) {
            return singleton;
        }
        _mutex.lock();
        if (singleton != nullptr) {
            _mutex.unlock();
            return singleton;
        }
        singleton = new SingleLazy();
        _mutex.unlock();
        return singleton;
        
    }
private:
    static SingleLazy* singleton;
    static std::mutex _mutex;
};

SingleLazy* SingleLazy::singleton = nullptr;
std::mutex SingleLazy::_mutex;
void test_singletonlazy() {
    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
         threads.emplace_back([](){
            SingleLazy* instance = SingleLazy::GetInstance();
            std::cout << "Instance address: " << instance << std::endl;
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

// 懒汉式 智能指针 自动回收
class SingleAuto {
private:
    SingleAuto(){}
    SingleAuto(const SingleAuto&) = delete;
    SingleAuto& operator=(const SingleAuto&) = delete;
public:
    ~SingleAuto() {std::cout << "single auto delete success " << std::endl; }
    static std::shared_ptr<SingleAuto> GetInstance() {
        if(singleton != nullptr) { // 1
            return singleton;
        }
        _mutex.lock();
        if (singleton != nullptr) {
            _mutex.unlock();
            return singleton;
        }
        singleton = std::shared_ptr<SingleAuto>(new SingleAuto); // 4
        _mutex.unlock();
        return singleton;
        
    }
private:
    static std::shared_ptr<SingleAuto> singleton;
    static std::mutex _mutex;
};

std::shared_ptr<SingleAuto> SingleAuto::singleton = nullptr;
std::mutex SingleAuto::_mutex;
void test_singletonauto() {
    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
         threads.emplace_back([](){
            auto instance = SingleAuto::GetInstance();
            std::cout << "Instance address: " << instance << std::endl;
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

// 内存模型修复
class SingleAutoMemory {
private:
    SingleAutoMemory(){}
    SingleAutoMemory(const SingleAutoMemory&) = delete;
    SingleAutoMemory& operator=(const SingleAutoMemory&) = delete;
public:
    ~SingleAutoMemory() {std::cout << "single auto delete success " << std::endl; }
    static std::shared_ptr<SingleAutoMemory> GetInstance() {
        if (_b_init.load(std::memory_order_acquire)) {
            return singleton;
        }
        _mutex.lock();
        if (_b_init.load(std::memory_order_relaxed)) {
            _mutex.unlock();
            return singleton;
        }
        singleton = std::shared_ptr<SingleAutoMemory>(new SingleAutoMemory);
        _b_init.store(true, std::memory_order_release);
        _mutex.unlock();
        return singleton;
        
    }
private:
    static std::shared_ptr<SingleAutoMemory> singleton;
    static std::mutex _mutex;
    static std::atomic<bool> _b_init;
};

std::shared_ptr<SingleAutoMemory> SingleAutoMemory::singleton = nullptr;
std::mutex SingleAutoMemory::_mutex;
std::atomic<bool> SingleAutoMemory::_b_init(false);
// SingleAuto类1和4存在线程安全
void test_singletonauto_memory(){
    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
         threads.emplace_back([](){
            auto instance = SingleAutoMemory::GetInstance();
            std::cout << "Instance address: " << instance << std::endl;
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

// 定义友元类帮忙回收
class SingleAutoSafe;
class SafeDeletor {
public:
	void operator()(SingleAutoSafe* sf) {
		std::cout << "this is safe deleter operator()" << std::endl;
		delete sf;
	}
};

class SingleAutoSafe {
private:
    SingleAutoSafe(){}
    SingleAutoSafe(const SingleAutoSafe&) = delete;
    SingleAutoSafe& operator=(const SingleAutoSafe&) = delete;
    ~SingleAutoSafe() {std::cout << "single auto delete success " << std::endl; }
    friend class SafeDeletor;
public:
    static std::shared_ptr<SingleAutoSafe> GetInstance() {
        if(singleton != nullptr) {
            return singleton;
        }
        _mutex.lock();
        if (singleton != nullptr) {
            _mutex.unlock();
            return singleton;
        }
        singleton = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, SafeDeletor());
        _mutex.unlock();
        return singleton;
        
    }
private:
    static std::shared_ptr<SingleAutoSafe> singleton;
    static std::mutex _mutex;
};

std::shared_ptr<SingleAutoSafe> SingleAutoSafe::singleton = nullptr;
std::mutex SingleAutoSafe::_mutex;
void test_singletonautosafe() {
    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
         threads.emplace_back([](){
            auto instance = SingleAutoSafe::GetInstance();
            std::cout << "Instance address: " << instance << std::endl;
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

// 使用call_once
class SingleOnce {
private:
    SingleOnce() = default;
    SingleOnce(const SingleOnce&) = delete;
    SingleOnce& operator=(const SingleOnce&) = delete;
public:
    static std::shared_ptr<SingleOnce> GetInstance() {
        static std::once_flag _flag;
        std::call_once(_flag, [&](){
            singleton = std::shared_ptr<SingleOnce>(new SingleOnce);
        });
        return singleton;
    }
    ~SingleOnce() {std::cout << "this is singleton destruct" << std::endl;}
private:
    static std::shared_ptr<SingleOnce> singleton;
};

std::shared_ptr<SingleOnce> SingleOnce::singleton = nullptr;
void test_singletononce() {
    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
         threads.emplace_back([](){
            auto instance = SingleOnce::GetInstance();
            std::cout << "Instance address: " << instance << std::endl;
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

template <typename T>
class Singleton {
protected:
	Singleton() = default;
	Singleton(const Singleton<T>&) = delete;
	Singleton& operator=(const Singleton<T>& st) = delete;
public:
	static std::shared_ptr<T> GetInstance() {
		static std::once_flag _flag;
		std::call_once(_flag, [&]() {
			_instance = std::shared_ptr<T>(new T);
			});
		return _instance;
	}
	void PrintAddress() {
		std::cout << _instance.get() << std::endl;
	}
	~Singleton() {
		std::cout << "this is singleton destruct" << std::endl;
	}
private:
    static std::shared_ptr<T> _instance;
};
template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;
// 网络编程中逻辑单例类
class LogicSystem :public Singleton<LogicSystem> {
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem(){}
private:
	LogicSystem(){}
};

void test_logic_system_singleton() {
    const int numThreads = 5;
    std::thread threads[numThreads];
    for (int i = 0; i < numThreads; ++i) {
        threads[i] = std::thread([](){
            auto logicInstance = LogicSystem::GetInstance();
            logicInstance->PrintAddress();
        });
    }

    for (int i = 0; i < numThreads; ++i) {
        threads[i].join();
    }
}

int main() {
    // test_singleton();
    // test_singletonhungry();
    // test_singletonlazy();
    // test_singletonauto();
    test_singletonauto_memory();
    // test_singletonautosafe();
    // test_singletononce();
    // test_logic_system_singleton();
    return 0;
}