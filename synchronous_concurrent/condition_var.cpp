#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>

int num = 1;
std::mutex  mtx_num;
void PoorImplemention() {
    std::thread t1([](){
        while(true){
            {
                std::lock_guard<std::mutex> lk(mtx_num);
                if (num == 1){
                    std::cout << "thread A print 1....." << std::endl;
                    num++;
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    std::thread t2([](){
        while(true){
            {
                std::lock_guard<std::mutex> lk(mtx_num);
                if (num == 2){
                    std::cout << "thread B print 2....." << std::endl;
                    num--;
                    continue;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
    t1.join();
	t2.join();
}
std::condition_variable  cvA;
std::condition_variable  cvB;
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

#include <queue>
#include <memory>
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

void test_safe_queue() {
    threadsafe_queue<int> safe_queue;
    std::mutex mtx_print;
    std::thread producer([&](){
        for (int i = 0; ; i++) {
            safe_queue.push(i);
            {
                std::lock_guard<std::mutex> printlk(mtx_print);
                std::cout << "producer push data is " << i << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    std::thread consumer_1([&](){
        for (; ;) {
            auto data = safe_queue.wait_and_pop();
            {
                std::lock_guard<std::mutex> printlk(mtx_print);
                std::cout << "consumer_1 wait and pop data is " << *data << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    std::thread consumer_2([&](){
        for (; ;) {
            auto data = safe_queue.try_pop();
            if (data != nullptr){
                {
                    std::lock_guard<std::mutex> printlk(mtx_print);
                    std::cout << "consumer_2 try pop data is " << *data << std::endl;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
    producer.join();
	consumer_1.join();
	consumer_2.join();
}

int main()
{
    // PoorImplemention();
    // ResonableImplemention();
    test_safe_queue();
    return 0;
}