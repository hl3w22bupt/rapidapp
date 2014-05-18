#include "rapidapp_core.h"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <cassert>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

DEFINE_string(url, "0.0.0.0:80", "server listen url, format: [ip:port]");
DEFINE_string(log_file, "", "logging file name");
DEFINE_int32(fps, 1000, "frame-per-second, MUST >= 1, associated with reload and timer");

namespace rapidapp {

const int MAX_TCP_BACKLOG = 102400;

bool AppLauncher::running_ = false;
bool AppLauncher::reloading_ = false;

AppLauncher::AppLauncher() : event_base_(NULL), internal_timer_(NULL),
                                listener_(NULL), app_(NULL)
{
    memset(&setting_, 0, sizeof(setting_));
}

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

int AppLauncher::InitLogging(int argc, char** argv)
{
    assert(argc > 0 && argv != NULL);

    char* file_name = setting_.log_file_name;
    if ('\0' == file_name[0])
    {
        file_name = argv[0];
    }

    google::InitGoogleLogging(file_name);
    google::SetLogDestination(google::INFO, file_name);
    google::InstallFailureFunction(&failed_cb_func);

    return 0;
}

int AppLauncher::Init(int argc, char** argv)
{
    // command line
    int ret = ParseCmdLine(argc, argv);
    if (ret != 0)
    {
        fprintf(stderr, "ParseCmdLine failed, argc:%d", argc);
        return -1;
    }

    // glog
    ret = InitLogging(argc, argv);
    if (ret != 0)
    {
        fprintf(stderr, "Init logger failed");
        return -1;
    }

    // signal
    InitSignalHandle();

#ifdef _DEBUG
    event_enable_debug_mode();
    //event_enable_debug_logging(EVENT_DBG_NONE);
#endif

    // event base
    struct event_base* evt_base = event_base_new();
    if (NULL == evt_base)
    {
        PLOG(ERROR)<<"create event base failed";
        return -1;
    }

    event_base_ = evt_base;

    // 打印libevent信息
#ifdef _DEBUG
    const char** methods = event_get_supported_methods();
    fprintf(stderr, "libevent version:%s, supported method:",
            event_get_version());
    for (int i=0; methods[i] != NULL; ++i)
    {
        fprintf(stderr, " %s ", methods[i]);
    }
    fprintf(stderr, "\n");
#endif

    // 帧计时器
    struct event* internal_timer_ = event_new(event_base_, -1, EV_PERSIST,
                                              internal_timer_cb_func, this);
    if (NULL == internal_timer_)
    {
        PLOG(ERROR)<<"create internal timer failed";
        return -1;
    }

    // 可配置，通过此项配置控制后台服务的帧率
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000000 / setting_.fps;
    if (event_add(internal_timer_, &tv) != 0)
    {
        PLOG(ERROR)<<"add internal timer to event base failed";
        return -1;
    }

    // TODO ctrl unix socket

    // listener, TODO port
    struct sockaddr_in listen_sa;
    listen_sa.sin_family = AF_INET;
    listen_sa.sin_addr.s_addr = inet_addr(setting_.listen_url);
    listen_sa.sin_port = htons(8080);
    listener_ = evconnlistener_new_bind(
         event_base_, socket_listen_cb_function, this,
         LEV_OPT_CLOSE_ON_FREE|LEV_OPT_CLOSE_ON_EXEC|LEV_OPT_REUSEABLE,
         MAX_TCP_BACKLOG, (const struct sockaddr*)&listen_sa,
         sizeof(listen_sa));
    if (NULL == listener_)
    {
        PLOG(ERROR)<<"create evlisten failed";
        return -1;
    }

    return 0;
}

int AppLauncher::CleanUp()
{
    if (listener_ != NULL)
    {
        evconnlistener_free(listener_);
        listener_ = NULL;
    }

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

    if (reloading_)
    {
        Reload();
        reloading_ = false;
    }

    if (!running_)
    {
        PLOG(INFO)<<"exit loop";
        event_base_loopbreak(event_base_);
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

int AppLauncher::ParseCmdLine(int argc, char** argv)
{
    assert(argc > 0 && argv != NULL);

    gflags::SetVersionString(app_->GetAppVersion());
    gflags::SetUsageMessage("server application based on rapidapp");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // listen url
    if (FLAGS_url.length() >= sizeof(setting_.listen_url))
    {
        fprintf(stderr, "url(length:%u) is too long\n", (unsigned)FLAGS_url.length());
        return -1;
    }
    strcpy(setting_.listen_url, FLAGS_url.c_str());

    // log file
    if (FLAGS_log_file.length() >= sizeof(setting_.log_file_name))
    {
        fprintf(stderr, "log_file(length:%u) is too long\n", (unsigned)FLAGS_log_file.length());
        return -1;
    }
    strcpy(setting_.log_file_name, FLAGS_log_file.c_str());

    if (FLAGS_fps < 1)
    {
        fprintf(stderr, "fps MUST >= 1");
        return -1;
    }
    setting_.fps = FLAGS_fps;

    return 0;
}

int AppLauncher::Run(RapidApp* app, int argc, char** argv)
{
    if (NULL == app)
    {
        fprintf(stderr, "app is NULL\n");
        return -1;
    }

    app_ = app;

    if (argc <=0 || NULL == argv)
    {
        fprintf(stderr, "argc:%d, argv:%p\n", argc, argv);
        return -1;
    }

    // 1. Init
    int ret = Init(argc, argv);
    if (ret != 0)
    {
        PLOG(ERROR)<<"Init failed, return"<<ret;
        return -1;
    }

    // 2. app init
    ret = app_->OnInit();
    if (ret != 0)
    {
        PLOG(ERROR)<<"app OnInit failed return:"<<ret;
        return -1;
    }

    LOG(INFO)<<"start success...";
#ifdef _DEBUG
    assert(event_base_ != NULL);
    event_base_dump_events(event_base_, stdout);
#endif

    // 3. mainloop
    running_ = true;
    event_base_dispatch(event_base_);

    // 4. app fini
    ret = app_->OnFini();
    if (ret != 0)
    {
        PLOG(ERROR)<<"app OnFini failed return:"<<ret;
    }

    // 5. CleanUp
    ret = CleanUp();
    if (ret != 0)
    {
        PLOG(ERROR)<<"CleanUp failed return:"<<ret;
    }

    return 0;
}

}
