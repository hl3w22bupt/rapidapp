#ifndef RAPIDAPP_FRAMEWORK_IMP_H_
#define RAPIDAPP_FRAMEWORK_IMP_H_

#include "rapidapp_interface.h"
#include "rapidapp_framework.h"
#include "rapidapp_net_mgr.h"
#include "rapidapp_easy_net.h"
#include "event2/event.h"
#include "event2/listener.h"
#include "event2/bufferevent.h"
#include <cstdio>
#include <csignal>

namespace rapidapp {

const int MAX_URL_LEN = 128;
const int MAX_FILE_NAME_LEN = 128;

struct AppSetting {
    int  fps;       // 每秒帧率
    char listen_uri[MAX_URL_LEN];
    char log_file_name[MAX_FILE_NAME_LEN];
};

class AppFrameWork : public IFrameWork {
    public:
        AppFrameWork();
        ~AppFrameWork();

    public:
        int Init(RapidApp* app, int argc, char** argv);
        int CleanUp();
        void MainLoop();

    // 对外接口虚函数实现类
    public:
        virtual EasyNet* CreateBackEnd(const char* url, IEventListener* event_listener);
        virtual void DestroyBackEnd(EasyNet** net);

    public:
        virtual int SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size);
        virtual int SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size);

    // 事件回调
    public:
        static void internal_timer_cb_func(evutil_socket_t fd, short what, void *arg) {
            if (NULL == arg)
                return;

            // 帧驱动逻辑
            static_cast<AppFrameWork*>(arg)->Tick();
        }

        static void socket_listen_cb_function(struct evconnlistener *listener,
                                              evutil_socket_t sock,
                                              struct sockaddr *addr, int len, void *ptr) {
            // 接收的新连接加入event管理
            if (NULL == ptr)
            {
                return;
            }
            else if (NULL == listener || sock < 0 || NULL == addr)
            {
                return;
            }

            // OnFrontEndConnect
            static_cast<AppFrameWork*>(ptr)->OnFrontEndConnect(sock, addr);
        }

        static void on_frontend_data_cb_func(struct bufferevent* bev, void *arg) {
            if (NULL == bev || NULL == arg)
                return;

            // OnFrontEndMsg
            static_cast<AppFrameWork*>(arg)->OnFrontEndMsg(bev);
        }

        static void on_nondata_event_cb_func(struct bufferevent* bev, short events, void *arg) {
            if (NULL == bev || NULL == arg)
                return;

            // OnFrontEndSocketEvent
            static_cast<AppFrameWork*>(arg)->OnFrontEndSocketEvent(bev, events);
        }


        static void failed_cb_func() {
        }

        static void on_ctrl_cb_func(struct bufferevent* bev, void *arg) {
            if (NULL == bev || NULL == arg)
                return;

            static_cast<AppFrameWork*>(arg)->OnCtrlMsg(bev);
        }

        static void signal_stop_handler(int signum, siginfo_t *sig_info, void *arg) {
            running_ = false;
        }

        static void signal_reload_handler(int signum, siginfo_t *sig_info, void *arg) {
            reloading_ = false;
        }

    private:
        // 作为异步event回调使用
        int Tick();
        int Reload();
        int OnCtrlMsg(struct bufferevent* bev);
        int OnFrontEndConnect(evutil_socket_t sock, struct sockaddr *addr);

        int OnFrontEndMsg(struct bufferevent* bev);
        int OnFrontEndSocketEvent(struct bufferevent* bev, short events);

    private:
        void InitSignalHandle();
        int InitLogging(int argc, char** argv);
        int ParseCmdLine(int argc, char** argv);

    private:
        struct event_base* event_base_;
        struct event* internal_timer_;          // 内部定时器，帧驱动
        struct evconnlistener* listener_;
        struct bufferevent* udp_ctrl_keeper_;   // command control mode

    private:
        RapidApp* app_;
        AppSetting setting_;

    private:
        static bool running_;
        static bool reloading_;

    private:
        class NetHandlerMgr frontend_handler_mgr_;
        class NetHandlerMgr backend_handler_mgr_;
};

}

#endif
