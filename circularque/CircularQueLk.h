#ifndef __CIRCULAR_QUE_LK_H
#define __CIRCULAR_QUE_LK_H

#include <iostream>
#include <mutex>
#include <memory>

template<typename T, size_t Cap>
class CircularQueLk:private std::allocator<T> {
public:
    CircularQueLk()
        :_max_size(Cap + 1),
         _data(std::allocator<T>::allocate(_max_size)), 
         _head(0), 
         _tail(0)
        {}
    CircularQueLk(const CircularQueLk&) = delete;
    CircularQueLk& operator=(const CircularQueLk&) volatile = delete; // 为什么拷贝复制有两个(可不写):volatile是一个线程修改了的结果可以被另一个线程看到
    CircularQueLk& operator=(const CircularQueLk&) = delete;
    ~CircularQueLk() {
        std::lock_guard<std::mutex> lock(_mtx);
        while(_head != _tail) {
            std::allocator<T>::destroy(_data + _head);
            _head = (_head + 1) % _max_size;
        }
        std::allocator<T>::deallocate(_data, _max_size);
    }

    template<typename ...Args>
    bool emplace(Args&& ...args) {
        std::lock_guard<std::mutex> lock(_mtx);
        if ((_tail + 1) % _max_size == _head) {
            std::cout << "circular que full !\n";
            return false;
        }
        // 尾部位置构造一个对象
        std::allocator<T>::construct(_data + _tail, std::forward<Args>(args)...);
        _tail = (_tail + 1) % _max_size;
        return true;
    }
    // 接受左值引用版本（加const：让其接受const类型也可以接受非const类型）
    bool push(const T& val) {
        std::cout << "called push const T& version\n";
        return emplace(val);
    }

    bool push(T&& val) {
        std::cout << "called push T&& version\n";
        return emplace(std::move(val));
    }

    bool pop(T& val) {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_head == _tail) {
            std::cout << "circular que empty !\n";
            return false;
        }
        val = std::move(_data[_head]);
        _head = (_head + 1) % _max_size;
        return true;
    }

private:
    size_t _max_size;
    T* _data;
    std::mutex _mtx;
    size_t _head = 0;
    size_t _tail = 0;
};

#endif