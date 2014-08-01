#ifndef RAPIDAPP_EASY_NET_H_
#define RAPIDAPP_EASY_NET_H_

#include "event2/event.h"
#include "event2/bufferevent.h"
#include "rapidapp_framework.h"

namespace rapidapp {

const int MAX_URL_LEN = 128;

enum NetState {
    NET_INIT        = 1,
    NET_CONNECTING  = 2,
    NET_ESTABLISHED = 3,
    NET_FAILED      = 4,
    NET_BEEN_CLOSED = 5,
};

enum SendError {
    EASY_NET_OK                       = 0,
    EASY_NET_ERR_NOT_YET_ESTABLISHED  = -1,
    EASY_NET_ERR_ESTABLISH_FAILED     = -2,
    EASY_NET_ERR_SEND_ERROR           = -3,
};

class AppFrameWork;
class NetHandlerMgr;
class EasyNet {
    public:
        EasyNet();
        ~EasyNet();

    public:
        int Init(evutil_socket_t sock_fd, int type, struct event_base* ev_base);
        int Connect(const char* uri, int type, struct event_base* ev_base);
        void CleanUp();

    public:
        int CreateUserContext(size_t uctx_size);
        void DestroyUserContext();

    public:
        int Send(const char* msg, size_t size);

    public:
        inline struct bufferevent* bufferevent() {
            return hevent_;
        }

        inline int net_type() {
            return net_type_;
        }

        inline void set_active_time(time_t now) {
            last_active_timestamp_ = now;
        }

        inline time_t last_active_time() const {
            return last_active_timestamp_;
        }

        inline void set_rpc_binded(void* rpc) {
            rpc_binded_ = rpc;
        }

        inline void* rpc_binded() {
            return rpc_binded_;
        }

        inline bool is_rpc_binded() {
            return rpc_binded_;
        }

        inline int nid() const {
            return nid_;
        }

        inline void* user_context() const {
            return user_context_;
        }

    private:
        struct bufferevent* hevent_;    // net对应的bufferevent实例
        int net_type_;                  // 网络实体类型
        char uri_[MAX_URL_LEN];         // 发起后端连接时，后端服务uri
        int nid_;                       // net id
        int state_;                     // state
        time_t last_active_timestamp_;  // 最近1次活跃时间点

    private:
        void* rpc_binded_;              // 捆绑的rpc

    private:
        void* user_context_;

        friend class AppFrameWork;
        friend class NetHandlerMgr;
};

}

#endif
