#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

namespace messaging {
    struct message_base { // 队列项的基础类
        virtual ~message_base(){}
    };

    template<typename Msg>
    struct wrapped_message :message_base { // 每个消息类型都需要特化
        Msg contents;
        explicit wrapped_message(Msg const& contents_) : contents(contents_) {}
    };

    class queue {
    public:
        template<typename T>
        void push(T const& msg) {
            std::lock_guard<std::mutex> lk(m);
            q.push(std::make_shared<wrapped_message<T>>(msg)); // 包装已传递的信息，存储指针
            cv.notify_all();
        }

        std::shared_ptr<message_base> wait_and_pop() {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [&]() {return !q.empty();}); // 当队列为空时阻塞
            auto res = q.front();
            q.pop();
            return res;
        }
    private:
        std::mutex m;
        std::condition_variable cv;
        std::queue<std::shared_ptr<message_base>> q; // 实际存储指向message_base类指针的队列
    };
}
#endif /*_MESSAGE_H_*/