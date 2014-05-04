#ifndef RAPIDAPP_LIBEVENT_H_
#define RAPIDAPP_LIBEVENT_H_

#include <event2/event.h>

namespace rapidapp {

class BaseEvent {
    public:
        BaseEvent(){};
        virtual ~BaseEvent(){};

    public:
        virtual int OnEventActive();
};

class IOEvent : public BaseEvent {
};

class TimerEvent : public BaseEvent {
};

class SignalEvent : public BaseEvent {
};

}

#endif
