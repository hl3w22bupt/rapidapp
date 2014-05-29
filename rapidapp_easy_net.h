#ifndef RAPIDAPP_EASY_NET_H_
#define RAPIDAPP_EASY_NET_H_

#include "event2/bufferevent.h"

namespace rapidapp {

class EasyNet {
    public:
        EasyNet();
        ~EasyNet();

    public:
        int Init();
        void CleanUp();

    public:
        int Send(const char* msg, size_t size);

    private:
        struct bufferevent* hevent_;
};

}

#endif
