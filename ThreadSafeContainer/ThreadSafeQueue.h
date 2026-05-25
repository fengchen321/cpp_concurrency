#pragma once
#include <queue>
#include <mutex>
#include <memory>
#include <condition_variable>
// 面临线程执行wait_and_pop时如果出现了异常，导致数据被滞留在队列中，其他线程也无法被唤醒的情况。
template<typename T>
class threadsafe_queue {
public:
    threadsafe_queue() {}

    threadsafe_queue(const threadsafe_queue& other) {
        std::lock_guard<std::mutex> lock(other.m_tx);
        data = other.data;
    }

    threadsafe_queue& operator=(const threadsafe_queue&) = delete;

public:
    void push(T new_value) {
        std::lock_guard<std::mutex> lock(m_tx);
        data.push(std::move(new_value));
        cv.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(m_tx);
        cv.wait(lock, [this]() {
            return !data.empty();
        });
        value = std::move(data.front());
        data.pop();
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(m_tx);
        if(data.empty()) return false;
        value = std::move(data.front());
        data.pop();
        return true;
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lock(m_tx);
        cv.wait(lock, [this]() {
            return !data.empty();
        });
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.front())));
        data.pop();
        return res;
    }


    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lock(m_tx);
        if(data.empty()) return std::shared_ptr<T>();
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.front())));
        data.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_tx);
        return data.empty();
    }
private:
    std::queue<T> data;
    mutable std::mutex m_tx;
    std::condition_variable cv;
};

template<typename T>
class threadsafe_queue_ptr {
public:
    threadsafe_queue_ptr() {}

    threadsafe_queue_ptr(const threadsafe_queue_ptr& other) {
        std::lock_guard<std::mutex> lock(other.m_tx);
        data = other.data;
    }

    threadsafe_queue_ptr& operator=(const threadsafe_queue_ptr&) = delete;

public:
    void push(T new_value) {
        // 需要先构造智能指针，如果构造的过程失败了也就不会push到队列中，不会污染队列中的数据
        std::shared_ptr<T> temp_data(std::make_shared<T>(std::move(new_value)));
        std::lock_guard<std::mutex> lk(m_tx);
        data.push(temp_data);
        cv.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(m_tx);
        cv.wait(lock, [this]() {
            return !data.empty();
        });
        value = std::move(*data.front());
        data.pop();
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(m_tx);
        if(data.empty()) return false;
        value = std::move(*data.front());
        data.pop();
        return true;
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lock(m_tx);
        cv.wait(lock, [this]() {
            return !data.empty();
        });
        std::shared_ptr<T> res = data.front();
        // std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.front()))); 
        data.pop();
        return res;
    }
    
    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lock(m_tx);
        if(data.empty()) return std::shared_ptr<T>();
        std::shared_ptr<T> res = data.front();
        data.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_tx);
        return data.empty();
    }
private:
    std::queue<std::shared_ptr<T>> data;
    mutable std::mutex m_tx;
    std::condition_variable cv;
};

// 将push和pop操作分化为分别对尾和对首部的操作。对首和尾分别用不同的互斥量管理就可以实现真正意义的并发。
template<typename T>
class threadsafe_queue_ht {
private:
    struct node {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
public:
    threadsafe_queue_ht(): head(new node), tail(head.get()) {}

    threadsafe_queue_ht(const threadsafe_queue_ht& other) =delete;

    threadsafe_queue_ht& operator=(const threadsafe_queue_ht&) = delete;

public:
    void push(T new_value) {
        std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data = new_data;
            node* const new_tail = p.get();
            tail->next = std::move(p);
            tail = new_tail;
        }
        cv.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_ptr<node> const old_head = wait_pop_head(value);
    }

    bool try_pop(T& value) {
        std::unique_ptr<node> const old_head = try_pop_head(value);
        return old_head;
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_ptr<node> const old_head = wait_pop_head();
        return old_head->data;
    }
    
    std::shared_ptr<T> try_pop() {
        std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }

    bool empty() const {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        return (head.get() == get_tail());
    }
private:
    node* get_tail() {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }

    std::unique_ptr<node> pop_head() {
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

    std::unique_lock<std::mutex> wait_for_data() {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        cv.wait(head_lock,[&] {return head.get() != get_tail(); });
        return std::move(head_lock);   
    }

    std::unique_ptr<node> wait_pop_head() {
        std::unique_lock<std::mutex> head_lock(wait_for_data());   
        return pop_head();
    }
    std::unique_ptr<node> wait_pop_head(T& value) {
        std::unique_lock<std::mutex> head_lock(wait_for_data());  
        value = std::move(*head->data);
        return pop_head();
    }

    std::unique_ptr<node> try_pop_head() {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }
    std::unique_ptr<node> try_pop_head(T& value) {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        value = std::move(*head->data);
        return pop_head();
    }
private:
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    std::condition_variable cv;
};
