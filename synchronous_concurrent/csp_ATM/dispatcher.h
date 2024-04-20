#ifndef _DISPATCH_H_
#define _DISPATCH_H_

#include "message.h"

namespace messaging {
    class close_queue {};

    template<typename PreviousDispatcher, typename Msg, typename Func>
    class TemplateDispatcher; // 提前声明 TemplateDispatcher 类模板

    class dispatcher {
    private:
        dispatcher(dispatcher const&) = delete;
        dispatcher& operator=(dispatcher const&) = delete;

        template<typename Dispatcher, typename Msg, typename Func>
        friend class TemplateDispatcher;

        void wait_and_dispatch() {
          for (;;) {
              auto msg = q->wait_and_pop();
              dispatch(msg);
          }
        }

        bool dispatch (std::shared_ptr<message_base> const& msg) {
          if (dynamic_cast<wrapped_message<close_queue>*>(msg.get())) {
              throw close_queue();
          }
          return false;
        }
    public:
        dispatcher(dispatcher&& other):q(other.q), chained(other.chained) {
          other.chained = true;
        }
        explicit dispatcher(queue* q_):q(q_), chained(false) {}
        // 函数返回一个类类型的局部变量时会先调用移动构造，如果没有移动构造再调用拷贝构造。
        template<typename Message, typename Func, typename Dispatcher>
        TemplateDispatcher<Dispatcher, Message, Func> handle(Func&& f, std::string info_msg) {
          return TemplateDispatcher<Dispatcher, Message, Func>(q, this, std::forward<Func>(f), info_msg);
        }
        
        ~dispatcher() noexcept(false) {
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
        sender():q(nullptr){}
        explicit sender(queue* q_):q(q_){}

        template<typename Message>
        void send(Message const& msg) {
          if (q) {
              q->push(msg);
          }
        }
    private:
        queue* q;
    };

    class receiver {
    public:
        operator sender() {
            return sender(&q);
        }
        dispatcher wait() {
            return dispatcher(&q);
        }
    private:
        queue q;
    };

    template<typename PreviousDispatcher, typename Msg, typename Func>
	  class TemplateDispatcher {
    private:
        TemplateDispatcher(TemplateDispatcher const&) = delete;
        TemplateDispatcher& operator=(TemplateDispatcher const&) = delete;
        template<typename Dispatcher, typename OtherMsg, typename OtherFunc>
        friend class TemplateDispatcher;

        void wait_and_dispatch() {
          for (;;){
              auto msg = q->wait_and_pop();
              if (dispatch(msg)){
                  break;
              }
          }
        }

        bool dispatch(std::shared_ptr<message_base> const& msg){
          if (wrapped_message<Msg>* wrapper = dynamic_cast<wrapped_message<Msg>*>(msg.get())) {
              f(wrapper->contents);
              return true;
          }
          else {
              return prev->dispatch(msg);
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