#ifndef __CIRCULAR_QUE_SYNC_H
#define __CIRCULAR_QUE_SYNC_H

#include <iostream>
#include <memory>
#include <atomic>
/*
将pop和push操作解耦;
将tail和head作为原子变量可以实现精细控制（轻量级Light）
*/
template<typename T, size_t Cap>
class CircularQueSync:private std::allocator<T> {
public:
    CircularQueSync()
        :_max_size(Cap + 1),
         _data(std::allocator<T>::allocate(_max_size)),
         _head(0), 
         _tail(0),
         _tail_update(0)
        {}
    CircularQueSync(const CircularQueSync&) = delete;
    CircularQueSync& operator=(const CircularQueSync&) volatile = delete;
    CircularQueSync& operator=(const CircularQueSync&) = delete;
    ~CircularQueSync() {
        while(_head != _tail) {
            std::allocator<T>::destroy(_data + _head);
            _head = (_head + 1) % _max_size;
        }
        std::allocator<T>::deallocate(_data, _max_size);
    }
    
    bool push(const T& val) {
        size_t t;
        do {
            t = _tail.load(std::memory_order_relaxed);
            if ((t + 1) % _max_size == _head.load(std::memory_order_acquire)) {
                std::cout << "circular que full !\n";
                return false;
            }
        } while(!_tail.compare_exchange_strong(t, (t + 1) % _max_size, std::memory_order_release, std::memory_order_relaxed));
        
        _data[t] = val;
        size_t tailup;
        do {
            tailup = t;
        } while(!_tail_update.compare_exchange_strong(tailup, (tailup + 1) % _max_size, std::memory_order_release, std::memory_order_relaxed));
        std::cout << "called push const T& version\n";
        return true;
    }

    bool pop(T& val) {
        size_t h;
        do {
            h = _head.load(std::memory_order_relaxed);  // 获取头部head值
            if(h == _tail.load(std::memory_order_acquire)) {
                std::cout << "circular que empty !\n";
                return false;
            }
            if(h == _tail_update.load(std::memory_order_acquire))
            {
                return false;
            }
            val = _data[h]; // 取出头部元素，不能使用std::move是因为只有一个线程pop成功
        } while(!_head.compare_exchange_strong(h, (h + 1) % _max_size, std::memory_order_release, std::memory_order_relaxed));
        return true;
    }

private:
    size_t _max_size;
    T* _data;
    std::atomic<size_t> _head; // 替换为原子变量
    std::atomic<size_t> _tail;
    std::atomic<size_t> _tail_update; // 标记尾部数据是否修改完成
};

#endif