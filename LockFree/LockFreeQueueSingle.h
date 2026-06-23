#pragma once
#include <memory>
#include <atomic>

namespace lockfreequeuesingle {
template <typename T>
class lockfree_queue {
private:
    struct node {
        std::shared_ptr<T> data;
        node* next;

        node() : next(nullptr) {}
    };

public:
    lockfree_queue() : head(new node), tail(head.load()) {}

    ~lockfree_queue() {
        while (node* const old_head = head.load()) {
            head.store(old_head->next);
            delete old_head;
        }
    }

    void push(T value) {
        std::shared_ptr<T> new_data(std::make_shared<T>(value));
        node* p = new node;
        node* const old_tail = tail.load();
        old_tail->data.swap(new_data);
        old_tail->next = p;
        tail.store(p);
    }

    std::shared_ptr<T> pop() {
        node* old_head = pop_head();
        if (!old_head) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res(old_head->data);
        delete old_head;
        return res;
    }

private:
    lockfree_queue(const lockfree_queue&) = delete;
    lockfree_queue& operator=(const lockfree_queue&) = delete;

    node* pop_head() {
        node* const old_head = head.load();
        if (old_head == tail.load()) {
            return nullptr;
        }
        head.store(old_head->next);
        return old_head;
    }

private:
    std::atomic<node*> head;
    std::atomic<node*> tail;
};

} // namespace lockfreequeuesingle