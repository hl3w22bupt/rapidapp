#ifndef RAPIDAPP_NET_H_
#define RAPIDAPP_NET_H_

#include <event2/event.h>

namespace rapidapp {

class AppLauncher;
class EasyNet {
    public:
        static EasyNet* CreateFrontEnd(const char* url);
        static EasyNet* CreateBackEnd(const char* url);

        static void DestroyFrontEnd(EasyNet** net);
        static void DestroyBackEnd(EasyNet** net);

    public:
        int SendToFrontEnd(const char* buf, size_t buf_size);
        int SendToBackEnd(const char* buf, size_t buf_size);

        // TODO SendvTo

    private:
        EasyNet();
        ~EasyNet();

        friend class AppLauncher;
};

}

#endif
