#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

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

int main() {
    Channel<int> ch(10);
    std::thread producer([&]() {
        for (int i = 0; i < 20; ++i) {
            ch.send(i);
            std::cout << "Sent: " << i << std::endl;
        }
        ch.close();
    });
    std::thread consumer([&]() {
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        int val;
        while(ch.receive(val)) {
            std::cout << "Received: " << val << std::endl;
        }
    });

    producer.join();
    consumer.join();
    return 0;
}