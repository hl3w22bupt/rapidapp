#ifndef RAPIDAPP_CORE_H_
#define RAPIDAPP_CORE_H_

#include "rapidapp_base.h"
#include "event2/event.h"
#include <cstdio>

namespace rapidapp {

class AppLauncher {
    public:
        AppLauncher();
        ~AppLauncher();

    public:
        int Run(RapidApp* app);

    public:
        static void internal_timer_cb_func(evutil_socket_t fd, short what, void *arg) {
#ifdef _DEBUG
            fprintf(stderr, "internal timer cb\n");
#endif
        }

    private:
        int Init();
        int CleanUp();

        // 作为异步event回调使用
        int Proc();
        int Tick();
        int Reload();
        int CtrlKeeper();

    private:
        struct event_base* event_base_;
        struct event* internal_timer_;      // 内部定时器，帧驱动

    private:
        RapidApp* app_;

    private:
        bool running_;
};

}

#endif
