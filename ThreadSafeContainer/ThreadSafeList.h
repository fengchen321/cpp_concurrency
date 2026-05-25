#pragma once
#include <mutex>
#include <memory>

template<typename T>
class threadsafe_list {
    struct node {
        std::mutex mtx;
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
        node() : next(nullptr) {}
        node(T const& value) : data(std::make_shared<T>(value)), next(nullptr) {}
    };
    node head;
public:
    threadsafe_list() {}
    ~threadsafe_list() {
        remove_if([](node const&) { return true; });
    }

    threadsafe_list(const threadsafe_list&) = delete;
    threadsafe_list& operator=(const threadsafe_list&) = delete;

    void push_front(T const& value) {
        std::unique_ptr<node> new_node = std::make_unique<node>(value);
        std::lock_guard<std::mutex> lk(head.mtx);
        new_node->next = std::move(head.next);
        head.next = std::move(new_node);
    }

    template<typename Function>
    void for_each(Function f) {
        node* current = &head;
        std::unique_lock<std::mutex> lk(head.mtx);
        while (node* const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lk(next->mtx);
            lk.unlock();
            f(*next->data);
            current = next;
            lk = std::move(next_lk);
        }
    }

    template<typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate p) {
        node* current = &head;
        std::unique_lock<std::mutex> lk(head.mtx);
        while (node* const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lk(next->mtx);
            lk.unlock();
            if (p(*next->data)) {
                return next->data;
            }
            current = next;
            lk = std::move(next_lk);
        }
        return std::shared_ptr<T>();
    }

    template<typename Predicate>
    void remove_if(Predicate p) {
        node* current = &head;
        std::unique_lock<std::mutex> lk(head.mtx);
        while (node* const next = current->next.get()) {
            std::unique_lock<std::mutex> next_lk(next->mtx);
            if (p(*next->data)) {
                std::unique_ptr<node> old_next = std::move(current->next);
                current->next = std::move(next->next);
                next_lk.unlock();
            } else {
                lk.unlock();
                current = next;
                lk = std::move(next_lk);
            }
        }
    }
};