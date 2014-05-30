#ifndef RAPIDAPP_EASY_NET_H_
#define RAPIDAPP_EASY_NET_H_

#include "event2/event.h"
#include "event2/bufferevent.h"
#include "rapidapp_framework.h"

namespace rapidapp {

class AppFrameWork;
class EasyNet {
    public:
        EasyNet();
        ~EasyNet();

    public:
        int Init(const char* uri, IEventListener* event_listener,
                 struct event_base* ev_base);
        void CleanUp();

    public:
        int Send(const char* msg, size_t size);

    private:
        struct bufferevent* hevent_;
        friend class AppFrameWork;
};

}

#endif
