#pragma once
#include <memory>
#include <atomic>

template <typename T>
class lockfree_stack {
private:
    struct node {
        std::shared_ptr<T> data;
        node* next;
        node(T const& data_) : data(std::make_shared<T>(data_)) {}
    };
public:
    lockfree_stack() : head(nullptr), to_be_deleted(nullptr), threads_in_pop(0) {}

    void push(const T& value) {
        node* const new_node = new node(value);
        do {
            new_node->next = head.load();
        } while (!head.compare_exchange_weak(new_node->next, new_node));
    }

    // std::shared_ptr<T> pop() {
    //     node* old_head = nullptr;
    //     do {
    //         old_head = head.load();
    //     } while (old_head && !head.compare_exchange_weak(old_head, old_head->next));
    //     return old_head ? old_head->data : std::shared_ptr<T>();
    // }

    std::shared_ptr<T> pop() {
        ++threads_in_pop;
        node* old_head = nullptr;
        do {
            old_head = head.load();
            if (!old_head) {
                --threads_in_pop;
                return std::shared_ptr<T>();
            }
        } while (!head.compare_exchange_weak(old_head, old_head->next));
        std::shared_ptr<T> res;
        if (old_head) {
            res.swap(old_head->data);
        }
        try_reclaim(old_head);
        return res;
    }

private:
    lockfree_stack(const lockfree_stack&) = delete;
    lockfree_stack& operator=(const lockfree_stack&) = delete;

    static void delete_nodes(node* nodes) {
        while(nodes) {
            node* next = nodes->next;
            delete nodes;
            nodes = next;
        }
    };
    /*
    只有当确定没有其他线程在 pop 中（即 threads_in_pop == 1 且减后为0），才能安全删除某些节点，
    否则就把节点挂到待删链表里，留给最后一个退出 pop 的线程统一清理。
    */ 
    void try_reclaim(node* old_head) {
        if (threads_in_pop == 1) {
            node* nodes_to_delete = to_be_deleted.exchange(nullptr);
            if (!--threads_in_pop) {
                delete_nodes(nodes_to_delete);
            } else if (nodes_to_delete) {
                chain_pending_nodes(nodes_to_delete);
            }
            delete old_head;
        } else {
            chain_pending_node(old_head);
            --threads_in_pop;
        }
    };

    void chain_pending_nodes(node* nodes) {
        node* last = nodes;
        while (node* const next = last->next) {
            last = next;
        }
        chain_pending_nodes(nodes, last);
    }

    void chain_pending_nodes(node* first, node* last) {
        last->next = to_be_deleted;
        while (!to_be_deleted.compare_exchange_weak(last->next, first));
    }

    void chain_pending_node(node* n) {
        chain_pending_nodes(n, n);
    }
        

private:
    std::atomic<node*> head;
    std::atomic<node*> to_be_deleted; // 待删除节点的链表
    std::atomic<int> threads_in_pop;  // 正在执行pip的线程计数
};