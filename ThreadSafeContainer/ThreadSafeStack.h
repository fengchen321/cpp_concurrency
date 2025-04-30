#pragma once
#include <exception>
#include <stack>
#include <mutex>
#include <memory>

struct empty_stack: std::exception
{
    const char* what() const throw(){
        return "empty stack!";
    };
};

template<typename T>
class threadsafe_stack {
public:
    threadsafe_stack() {}

    threadsafe_stack(const threadsafe_stack& other) {
        std::lock_guard<std::mutex> lock(other.m_tx);
        data = other.data;
    }

    threadsafe_stack& operator=(const threadsafe_stack&) = delete;

public:
    void push(T new_value) {
        std::lock_guard<std::mutex> lock(m_tx);
        data.push(std::move(new_value));
    }

    std::shared_ptr<T> pop() {
        std::lock_guard<std::mutex> lock(m_tx);
        if (data.empty()) throw empty_stack();
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.top())));
        data.pop();
        return res;
    }

    void pop(T& value) {
        std::lock_guard<std::mutex> lock(m_tx);
        if (data.empty()) throw empty_stack();
        value = std::move(data.top());
        data.pop();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_tx);
        return data.empty();
    }
private:
    std::stack<T> data;
    mutable std::mutex m_tx;
};

#include <condition_variable>
template<typename T>
class threadsafe_stack_waitable {
public:
    threadsafe_stack_waitable() {}

    threadsafe_stack_waitable(const threadsafe_stack_waitable& other) {
        std::lock_guard<std::mutex> lock(other.m_tx);
        data = other.data;
    }

    threadsafe_stack_waitable& operator=(const threadsafe_stack_waitable&) = delete;

public:
    void push(T new_value) {
        std::lock_guard<std::mutex> lock(m_tx);
        data.push(std::move(new_value));
        cv.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(m_tx);
        cv.wait(lock, [this]() {
            if(data.empty()) {
                return false;
            }
            return true;
        });
        value = std::move(data.top());
        data.pop();
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(m_tx);
        if(data.empty()) return false;
        value = std::move(data.top());
        data.pop();
        return true;
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lock(m_tx);
        cv.wait(lock, [this]() {
            if(data.empty()) {
                return false;
            }
            return true;
        });
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.top())));
        data.pop();
        return res;
    }

    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lock(m_tx);
        if(data.empty()) return std::shared_ptr<T>();
        std::shared_ptr<T> const res(std::make_shared<T>(std::move(data.top())));
        data.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_tx);
        return data.empty();
    }
private:
    std::stack<T> data;
    mutable std::mutex m_tx;
    std::condition_variable cv;
};