#include "rapidapp_core.h"
#include <cassert>

namespace rapidapp {

AppLauncher::AppLauncher() : event_base_(NULL), internal_timer_(NULL),
                                app_(NULL), running_(false)
{}

AppLauncher::~AppLauncher()
{
    CleanUp();
}

int AppLauncher::Init()
{
    struct event_base* evt_base = event_base_new();
    if (NULL == evt_base)
    {
        return -1;
    }

    event_base_ = evt_base;

    const char** methods = event_get_supported_methods();
    fprintf(stderr, "libevent version:%s, supported method:",
            event_get_version());
    for (int i=0; methods[i] != NULL; ++i)
    {
        fprintf(stderr, " %s ", methods[i]);
    }
    fprintf(stderr, "\n");

    struct event* internal_timer_ = evtimer_new(event_base_,
                                               internal_timer_cb_func, this);
    if (NULL == internal_timer_)
    {
        return -1;
    }

    // TODO, 可配置
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    if (event_add(internal_timer_, &tv) != 0)
    {
        return -1;
    }

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

int AppLauncher::Proc()
{
    assert(app_ != NULL);

    // 1. poll msg from front/back
    // event_base_loop();
    return 0;
}

int AppLauncher::Tick()
{
    assert(app_ != NULL);
    return 0;
}

int AppLauncher::Reload()
{
    assert(app_ != NULL);
    return 0;
}

int AppLauncher::CtrlKeeper()
{
    assert(app_ != NULL);
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

    // 3. mainloop
#ifdef _DEBUG
    event_base_dump_events(event_base_, stdout);
#endif
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
