#ifndef _DISPATCH_H_
#define _DISPATCH_H_

#include "message.h"

namespace messaging {
    class close_queue {}; // 用于关闭队列的消息

    template<typename PreviousDispatcher, typename Msg, typename Func>
    class TemplateDispatcher; // 提前声明 TemplateDispatcher 类模板

    class dispatcher {
    private:
        dispatcher(dispatcher const&) = delete; // 不能拷贝构造
        dispatcher& operator=(dispatcher const&) = delete;

        template<typename Dispatcher, typename Msg, typename Func>
        friend class TemplateDispatcher; // 允许TemplateDispatcher实例访问内部成员

        void wait_and_dispatch() {
          for (;;) { // 循环，等待调度消息
              auto msg = q->wait_and_pop();
              dispatch(msg);
          }
        }

        bool dispatch (std::shared_ptr<message_base> const& msg) { // 检查close_queue消息然后抛出
          if (dynamic_cast<wrapped_message<close_queue>*>(msg.get())) {
              throw close_queue();
          }
          return false; // 表示消息没有被处理
        }
    public:
        dispatcher(dispatcher&& other):q(other.q), chained(other.chained) { // 可以移动构造
          other.chained = true; // 源不能等待消息
        }
        explicit dispatcher(queue* q_):q(q_), chained(false) {}
        // 函处理指定类型的消息：数返回一个class类型的局部变量时会先调用移动构造，如果没有移动构造再调用拷贝构造。
        template<typename Message, typename Func, typename Dispatcher>
        TemplateDispatcher<Dispatcher, Message, Func> handle(Func&& f, std::string info_msg) {
          return TemplateDispatcher<Dispatcher, Message, Func>(q, this, std::forward<Func>(f), info_msg);
        }
        
        ~dispatcher() noexcept(false) { // 析构函数可能抛出异常
          if (!chained) {
              wait_and_dispatch();
          }
        }
    private:
        queue* q;
        bool chained;
    };

    class sender {
    public:
        sender():q(nullptr){} // sender无队列（默认构造函数）
        explicit sender(queue* q_):q(q_){} // 从指向队列的指针进行构造

        template<typename Message>
        void send(Message const& msg) {
          if (q) {
              q->push(msg);  // 将发送信息推送给队列
          }
        }
    private:
        queue* q; // sender是一个队列指针的包装类，对sender实例的拷贝，只是拷贝指向队列的指针不是队列本身
    };

    class receiver {
    public:
        operator sender() { // （转换函数）允许将类中队列隐式转换为一个sender队列
            return sender(&q);
        }
        dispatcher wait() { // 等待对队列进行调度
            return dispatcher(&q);
        }
    private:
        queue q;  // 接受者拥有的队列
    };

    template<typename PreviousDispatcher, typename Msg, typename Func>
	  class TemplateDispatcher {
    private:
        TemplateDispatcher(TemplateDispatcher const&) = delete;
        TemplateDispatcher& operator=(TemplateDispatcher const&) = delete;
        template<typename Dispatcher, typename OtherMsg, typename OtherFunc>
        friend class TemplateDispatcher; // 所有特化的TemplateDispatcher类型实例都是友元类

        void wait_and_dispatch() {
          for (;;){
              auto msg = q->wait_and_pop();
              if (dispatch(msg)){  // 如果消息处理过后，跳出循环
                  break;
              }
          }
        }

        bool dispatch(std::shared_ptr<message_base> const& msg){
          if (wrapped_message<Msg>* wrapper = dynamic_cast<wrapped_message<Msg>*>(msg.get())) { // 检查消息类型，并且调用函数
              f(wrapper->contents);
              return true;
          }
          else {
              return prev->dispatch(msg); // 链接到之前的调度器上
          }
        }
    public:
        TemplateDispatcher(TemplateDispatcher&& other):
          q(other.q), prev(other.prev), f(std::move(other.f)),
          chained(other.chained), _msg(other._msg) {
              other.chained = true;
        }
        TemplateDispatcher(queue* q_, PreviousDispatcher* prev_, Func&& f_, std::string msg):
          q(q_), prev(prev_), f(std::forward<Func>(f_)), chained(false), _msg(msg) {
              prev_->chained = true;
        }

        template<typename OtherMsg, typename OtherFunc>
        TemplateDispatcher<TemplateDispatcher, OtherMsg, OtherFunc> handle(OtherFunc&& of,  std::string  info_msg) {
          return TemplateDispatcher<TemplateDispatcher, OtherMsg,OtherFunc>(q, this, std::forward<OtherFunc>(of), info_msg);
        }

        ~TemplateDispatcher()noexcept(false) {
          if (!chained){
              wait_and_dispatch();
          }
        }
    
    private:
        queue* q;
        PreviousDispatcher* prev;
        Func f;
        bool chained;
        std::string _msg;
  };
}

#endif /*_DISPATCH_H_*/