#ifndef RAPIDAPP_NET_H_
#define RAPIDAPP_NET_H_

#include <event2/event.h>

namespace rapidapp {

class EasyNet {
    public:
        static EasyNet* CreateFrontEnd(const char* url);
        static EasyNet* CreateBackEnd(const char* url);

    public:
        int SendToFrontEnd(const char* buf, size_t buf_size);
        int SendToBackEnd(const char* buf, size_t buf_size);

    private:
        EasyNet();
        ~EasyNet();
};

}

#endif
