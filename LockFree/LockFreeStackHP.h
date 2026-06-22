#pragma once

#include <atomic>
#include <memory>

#include "HazardPointer.h"

template <typename T>
class lockfree_stack_hp {
private:
    struct node {
        std::shared_ptr<T> data;
        node* next;
        node(T const& data_) : data(std::make_shared<T>(data_)) {}
    };
public:
    lockfree_stack_hp() : head(nullptr) {}

    void push(const T& value) {
        node* const new_node = new node(value);
        do {
            new_node->next = head.load();
        } while (!head.compare_exchange_weak(new_node->next, new_node));
    }

    std::shared_ptr<T> pop() {
        std::atomic<void*>& hp = get_hazard_pointer_for_current_thread();
        node* old_head = head.load();
        do {
            node* temp;
            do {
                temp = old_head;
                hp.store(old_head);
                old_head = head.load();
            } while (old_head != temp);
        } while (old_head && !head.compare_exchange_strong(old_head, old_head->next));
        hp.store(nullptr);

        std::shared_ptr<T> res;
        if (old_head) {
            res.swap(old_head->data);
            if (outstanding_hazard_pointers_for(old_head)) {
                reclaim_later(old_head);
            } else {
                delete old_head;
            }
            delete_nodes_with_no_hazards();
        }
        return res;
    }

private:
    lockfree_stack_hp(const lockfree_stack_hp&) = delete;
    lockfree_stack_hp& operator=(const lockfree_stack_hp&) = delete;

private:
    std::atomic<node*> head;
};