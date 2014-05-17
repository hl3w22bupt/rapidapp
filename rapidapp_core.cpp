#include "rapidapp_core.h"
#include <cassert>

namespace rapidapp {

bool AppLauncher::running_ = false;
bool AppLauncher::reloading_ = false;

AppLauncher::AppLauncher() : event_base_(NULL), internal_timer_(NULL),
                                app_(NULL)
{}

AppLauncher::~AppLauncher()
{
    CleanUp();
}

void AppLauncher::InitSignalHandle()
{
    signal(SIGHUP,  SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    struct sigaction sig_act;
    sig_act.sa_flags = SA_SIGINFO;
    sig_act.sa_sigaction = signal_stop_handler;
    sigaction(SIGTERM, &sig_act, NULL);
    sigaction(SIGINT, &sig_act, NULL);
    sigaction(SIGQUIT, &sig_act, NULL);
    sigaction(SIGUSR1, &sig_act, NULL);

    sig_act.sa_sigaction = signal_reload_handler;
    sigaction(SIGUSR2, &sig_act, NULL);
}

int AppLauncher::Init()
{
    InitSignalHandle();

#ifdef _DEBUG
    event_enable_debug_mode();
    //event_enable_debug_logging(EVENT_DBG_NONE);
#endif

    struct event_base* evt_base = event_base_new();
    if (NULL == evt_base)
    {
        return -1;
    }

    event_base_ = evt_base;

    // print libevent info
    const char** methods = event_get_supported_methods();
    fprintf(stderr, "libevent version:%s, supported method:",
            event_get_version());
    for (int i=0; methods[i] != NULL; ++i)
    {
        fprintf(stderr, " %s ", methods[i]);
    }
    fprintf(stderr, "\n");

    struct event* internal_timer_ = event_new(event_base_, -1, EV_PERSIST,
                                               internal_timer_cb_func, this);
    if (NULL == internal_timer_)
    {
        return -1;
    }

    // TODO, 可配置，通过此项配置控制后台服务的帧率
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    if (event_add(internal_timer_, &tv) != 0)
    {
        return -1;
    }

    // TODO ctrl unix socket

    return 0;
}

int AppLauncher::CleanUp()
{
    if (internal_timer_ != NULL)
    {
        event_free(internal_timer_);
        internal_timer_ = NULL;
    }

    if (event_base_ != NULL)
    {
        event_base_free(event_base_);
        event_base_ = NULL;
    }

    return 0;
}

int AppLauncher::Tick()
{
    assert(app_ != NULL);
    assert(event_base_ != NULL);

    if (!running_)
    {
#ifdef _DEBUG
        fprintf(stderr, "exit loop\n");
#endif
        event_base_loopbreak(event_base_);
    }

    if (reloading_)
    {
        Reload();
        reloading_ = false;
    }

    return 0;
}

int AppLauncher::Reload()
{
    assert(app_ != NULL);
    assert(event_base_ != NULL);

    return 0;
}

int AppLauncher::CtrlKeeper()
{
    assert(app_ != NULL);
    assert(event_base_ != NULL);

    return 0;
}

int AppLauncher::Run(RapidApp* app)
{
    if (NULL == app)
    {
        return -1;
    }

    app_ = app;

    // 1. Init
    int ret = Init();
    if (ret != 0)
    {
        return -1;
    }

    // 2. app init
    ret = app_->OnInit();
    if (ret != 0)
    {
        return -1;
    }

#ifdef _DEBUG
    event_base_dump_events(event_base_, stdout);
#endif

    // 3. mainloop
    running_ = true;
    event_base_dispatch(event_base_);

    // 4. app fini
    ret = app_->OnFini();
    if (ret != 0)
    {
        // log
    }

    // 5. CleanUp
    ret = CleanUp();
    if (ret != 0)
    {
        // log
    }

    return 0;
}

}
