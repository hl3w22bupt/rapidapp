#ifndef RAPIDAPP_EASY_NET_H_
#define RAPIDAPP_EASY_NET_H_

#include "event2/event.h"
#include "event2/bufferevent.h"
#include "rapidapp_framework.h"

namespace rapidapp {

const int MAX_URL_LEN = 128;

class AppFrameWork;
class EasyNet {
    public:
        EasyNet();
        ~EasyNet();

    public:
        int Init(evutil_socket_t sock_fd, int type, struct event_base* ev_base);
        int Connect(const char* uri, int type, struct event_base* ev_base);
        void CleanUp();

    public:
        int Send(const char* msg, size_t size);

    public:
        inline struct bufferevent* bufferevent() {
            return hevent_;
        }

        inline int net_type() {
            return net_type_;
        }

    private:
        struct bufferevent* hevent_;
        int net_type_;                  // 网络实体类型
        char uri_[MAX_URL_LEN];         // 发起后端连接时，后端服务uri
        friend class AppFrameWork;
};

}

#endif
