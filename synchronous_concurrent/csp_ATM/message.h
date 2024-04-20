#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

namespace messaging {
    struct message_base {
        virtual ~message_base(){}
    };

    template<typename Msg>
    struct wrapped_message :message_base {
        Msg contents;
        explicit wrapped_message(Msg const& contents_) : contents(contents_) {}
    };

    class queue {
    public:
        template<typename T>
        void push(T const& msg) {
            std::lock_guard<std::mutex> lk(m);
            q.push(std::make_shared<wrapped_message<T>>(msg));
            cv.notify_all();
        }

        std::shared_ptr<message_base> wait_and_pop() {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [&]() {return !q.empty();});
            auto res = q.front();
            q.pop();
            return res;
        }
    private:
        std::mutex m;
        std::condition_variable cv;
        std::queue<std::shared_ptr<message_base>> q;
    };
}
#endif /*_MESSAGE_H_*/