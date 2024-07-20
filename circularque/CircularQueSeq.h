#ifndef __CIRCULAR_QUE_SEQ_H
#define __CIRCULAR_QUE_SEQ_H

#include <iostream>
#include <memory>
#include <atomic>
/*
将类的成员变量mutex换成atomic类型的原子变量, 
利用自旋锁的思路将锁替换为原子变量循环检测的方式，
达到锁住互斥逻辑效果。
*/
template<typename T, size_t Cap>
class CircularQueSeq:private std::allocator<T> {
public:
    CircularQueSeq()
        :_max_size(Cap + 1),
         _data(std::allocator<T>::allocate(_max_size)),
         _atomic_using(false),
         _head(0), 
         _tail(0)
        {}
    CircularQueSeq(const CircularQueSeq&) = delete;
    CircularQueSeq& operator=(const CircularQueSeq&) volatile = delete;
    CircularQueSeq& operator=(const CircularQueSeq&) = delete;
    ~CircularQueSeq() {
        // 析构这里不替换是因为对象生命周期最后阶段不需要竞争控制
        while(_head != _tail) {
            std::allocator<T>::destroy(_data + _head);
            _head = (_head + 1) % _max_size;
        }
        std::allocator<T>::deallocate(_data, _max_size);
    }

    template<typename ...Args>
    bool emplace(Args&& ...args) {
        bool use_expected = false;
        bool use_desired = true;
        do {
            use_desired = true;
            use_expected = false;
        } while(!_atomic_using.compare_exchange_strong(use_expected, use_desired));

        if ((_tail + 1) % _max_size == _head) {
            std::cout << "circular que full !\n";
            do {
                use_expected = true;
                use_desired = false;
            } while(!_atomic_using.compare_exchange_strong(use_expected, use_desired));
            return false;
        }
        // 尾部位置构造一个对象
        std::allocator<T>::construct(_data + _tail, std::forward<Args>(args)...);
        _tail = (_tail + 1) % _max_size;
        do {
            use_expected = true;
            use_desired = false;
        } while(!_atomic_using.compare_exchange_strong(use_expected, use_desired));
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
        bool use_expected = false;
        bool use_desired = true;
        do {
            use_desired = true;
            use_expected = false;
        } while(!_atomic_using.compare_exchange_strong(use_expected, use_desired));

        if (_head == _tail) {
            std::cout << "circular que empty !\n";
            do {
                use_expected = true;
                use_desired = false;
            } while(!_atomic_using.compare_exchange_strong(use_expected, use_desired));
            return false;
        }
        val = std::move(_data[_head]);
        _head = (_head + 1) % _max_size;
        do {
            use_expected = true;
            use_desired = false;
        } while(!_atomic_using.compare_exchange_strong(use_expected, use_desired));
        return true;
    }

private:
    size_t _max_size;
    T* _data;
    std::atomic<bool> _atomic_using;
    size_t _head = 0;
    size_t _tail = 0;
};

#endif