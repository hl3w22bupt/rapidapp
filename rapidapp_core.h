#ifndef RAPIDAPP_CORE_H_
#define RAPIDAPP_CORE_H_

#include "rapidapp_interface.h"
#include "rapidapp_net_mgr.h"
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

class AppLauncher {
    public:
        AppLauncher();
        ~AppLauncher();

    public:
        int Run(RapidApp* app, int argc, char** argv);

    public:
        static void internal_timer_cb_func(evutil_socket_t fd, short what, void *arg) {
            if (NULL == arg)
                return;

            // 帧驱动逻辑
            static_cast<AppLauncher*>(arg)->Tick();
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
            static_cast<AppLauncher*>(ptr)->OnFrontEndConnect(sock, addr);
        }

        static void on_frontend_data_cb_func(struct bufferevent* bev, void *arg) {
            if (NULL == bev || NULL == arg)
                return;

            // OnFrontEndMsg
            static_cast<AppLauncher*>(arg)->OnFrontEndMsg(bev);
        }

        static void on_nondata_event_cb_func(struct bufferevent* bev, short events, void *arg) {
            if (NULL == bev || NULL == arg)
                return;

            // OnFrontEndSocketEvent
            static_cast<AppLauncher*>(arg)->OnFrontEndSocketEvent(bev, events);
        }


        static void failed_cb_func() {
        }

        static void on_ctrl_cb_func(struct bufferevent* bev, void *arg) {
            if (NULL == bev || NULL == arg)
                return;

            static_cast<AppLauncher*>(arg)->OnCtrlMsg(bev);
        }

        static void signal_stop_handler(int signum, siginfo_t *sig_info, void *arg) {
            running_ = false;
        }

        static void signal_reload_handler(int signum, siginfo_t *sig_info, void *arg) {
            reloading_ = false;
        }

    private:
        EasyNet& GetNetEasyInstance() {
            return easy_net_handler_;
        }

    private:
        int Init(int argc, char** argv);
        int CleanUp();

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
        class ConnectionHandlerMgr connection_handler_mgr_;
        class EasyNet easy_net_handler_;
};

}

#endif
