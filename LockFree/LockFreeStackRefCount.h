#pragma once

#include <atomic>
#include <memory>

template <typename T>
class lockfree_stack_refcount {
private:
    struct node;
    struct counted_node_ptr {
        int external_count;
        node* ptr;

        counted_node_ptr() : external_count(0), ptr(nullptr) {}
        counted_node_ptr(T const& data_) : external_count(1), ptr(new node(data_)) {}
    };

    struct node {
        std::shared_ptr<T> data;
        std::atomic<int> internal_count;
        counted_node_ptr next;
        node(T const& data_) : data(std::make_shared<T>(data_)), internal_count(0), next() {}
    };

public:
    lockfree_stack_refcount() : head(counted_node_ptr()) {}

    ~lockfree_stack_refcount() {
        while(pop());
    }

    void push(const T& value) {
        counted_node_ptr new_node = counted_node_ptr(value);
        new_node.ptr->next = head.load();
        while (!head.compare_exchange_weak(new_node.ptr->next, new_node));
    }

    std::shared_ptr<T> pop() {
        counted_node_ptr old_head = head.load();
        for (;;) {
            increase_head_count(old_head);
            node* const ptr = old_head.ptr;
            if (!ptr) {
                return std::shared_ptr<T>();
            }
            if (head.compare_exchange_strong(old_head, ptr->next)) {
                std::shared_ptr<T> res;
                res.swap(ptr->data);
                int const count_increase = old_head.external_count - 2;
                if (ptr->internal_count.fetch_add(count_increase) == -count_increase) {
                    delete ptr;
                }
                return res;
            } else if (ptr->internal_count.fetch_sub(1) == 1) {
                delete ptr;
            }
        }
    }

private:
    lockfree_stack_refcount(const lockfree_stack_refcount&) = delete;
    lockfree_stack_refcount& operator=(const lockfree_stack_refcount&) = delete;

    void increase_head_count(counted_node_ptr& old_counter) {
        counted_node_ptr new_counter;
        do {
            new_counter = old_counter;
            ++new_counter.external_count;
        } while (!head.compare_exchange_weak(old_counter, new_counter));

        old_counter.external_count = new_counter.external_count;
    }

private:
    // counted_node_ptr 在 64 位系统上通常是 16 字节，std::atomic<counted_node_ptr>
    // 可能会生成 __atomic_*_16 调用，因此链接目标需要额外链接 libatomic。
    std::atomic<counted_node_ptr> head;
};