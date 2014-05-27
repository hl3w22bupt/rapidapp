#ifndef RAPIDAPP_EASY_NET_H_
#define RAPIDAPP_EASY_NET_H_

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
};

}

#endif
