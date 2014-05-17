#ifndef RAPIDAPP_CORE_H_
#define RAPIDAPP_CORE_H_

#include "rapidapp_base.h"
#include "event2/event.h"
#include <cstdio>
#include <csignal>

namespace rapidapp {

const int MAX_URL_LEN = 128;
const int MAX_FILE_NAME_LEN = 128;

struct AppSetting {
    char listen_url[MAX_URL_LEN];
    char log_conf_file[MAX_FILE_NAME_LEN];
};

class AppLauncher {
    public:
        AppLauncher();
        ~AppLauncher();

    public:
        int Run(RapidApp* app, int argc, char** argv);

    public:
        static void internal_timer_cb_func(evutil_socket_t fd, short what, void *arg) {
#ifdef _DEBUG
            fprintf(stderr, "internal timer cb, fd%d\n", fd);
#endif
            if (NULL == arg)
                return;

            static_cast<AppLauncher*>(arg)->Tick();
        }

        static void signal_stop_handler(int signum, siginfo_t *sig_info, void *arg) {
            running_ = false;
        }

        static void signal_reload_handler(int signum, siginfo_t *sig_info, void *arg) {
            reloading_ = false;
        }

    private:
        int Init();
        int CleanUp();

        // 作为异步event回调使用
        int Tick();
        int Reload();
        int CtrlKeeper();

    private:
        void InitSignalHandle();
        int ParseCmdLine(int argc, char** argv);

    private:
        struct event_base* event_base_;
        struct event* internal_timer_;      // 内部定时器，帧驱动

    private:
        RapidApp* app_;
        AppSetting setting_;

    private:
        static bool running_;
        static bool reloading_;
};

}

#endif
