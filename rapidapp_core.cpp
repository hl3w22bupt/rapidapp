#include "rapidapp_core.h"
#include "utils/rap_net_uri.h"
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <cassert>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

DEFINE_string(uri, "tcp:0.0.0.0:80", "server listen uri, format: [proto:ip:port]");
DEFINE_string(log_file, "", "logging file name");
DEFINE_int32(fps, 1000, "frame-per-second, MUST >= 1, associated with reload and timer");

namespace rapidapp {

const int MAX_TCP_BACKLOG = 102400;

const char* udp_uri = "udp:127.0.0.1:9090";

bool AppLauncher::running_ = false;
bool AppLauncher::reloading_ = false;

void libevent_log_cb_func(int severity, const char *msg) {
    if (msg != NULL) {
        PLOG(INFO)<<"libevent log:"<<msg;
    }
}

AppLauncher::AppLauncher() : event_base_(NULL), internal_timer_(NULL),
                                listener_(NULL), app_(NULL), frontend_handler_mgr_()
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
    assert(app_ != NULL);

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

    // set libevent log callback
    event_set_log_callback(libevent_log_cb_func);

    size_t up_size = 2 * app_->GetFrontEndMaxMsgSize();
    ret = frontend_handler_mgr_.Init(up_size);
    if (ret != 0)
    {
        LOG(ERROR)<<"init net mgr failed";
        return -1;
    }

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

    // ctrl udp socket
    evutil_socket_t udp_ctrl_sockfd = rap_uri_open_socket(udp_uri);
    if (udp_ctrl_sockfd < 0)
    {
        PLOG(ERROR)<<"open socket failed by uri:"<<udp_uri;
        return -1;
    }

    evutil_make_socket_nonblocking(udp_ctrl_sockfd);
    evutil_make_socket_closeonexec(udp_ctrl_sockfd);
    udp_ctrl_keeper_ = bufferevent_socket_new(event_base_, udp_ctrl_sockfd,
                                              BEV_OPT_CLOSE_ON_FREE);
    if (NULL == udp_ctrl_keeper_)
    {
        PLOG(ERROR)<<"create bufferevent for ctrl keeper failed";
        return -1;
    }
    bufferevent_enable(udp_ctrl_keeper_, EV_READ|EV_WRITE);
    bufferevent_setcb(udp_ctrl_keeper_, on_ctrl_cb_func, NULL, NULL, this);

    // tcp listener
    struct sockaddr_in listen_sa;
    ret = rap_uri_get_socket_addr(setting_.listen_uri, &listen_sa);
    if (ret != 0)
    {
        PLOG(ERROR)<<"bad uri: "<<setting_.listen_uri;
        return -1;
    }

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

    if (udp_ctrl_keeper_ != NULL)
    {
        bufferevent_free(udp_ctrl_keeper_);
        udp_ctrl_keeper_ = NULL;
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

int AppLauncher::OnCtrlMsg(struct bufferevent* bev)
{
    assert(app_ != NULL);
    assert(event_base_ != NULL);

    if (NULL ==bev)
    {
        PLOG(ERROR)<<"bev is null, but event triggered";
        return -1;
    }

    // TODO frame specified control msg
    app_->OnRecvCtrl();

    return 0;
}

int AppLauncher::OnFrontEndConnect(evutil_socket_t sock, struct sockaddr *addr)
{
    assert(sock >= 0 && addr != NULL);

    PLOG(INFO)<<"has accepted new connect:"<<
        inet_ntoa(((struct sockaddr_in*)addr)->sin_addr);

    evutil_make_socket_nonblocking(sock);
    evutil_make_socket_closeonexec(sock);
    struct bufferevent* event =
        bufferevent_socket_new(event_base_, sock,
                               BEV_OPT_CLOSE_ON_FREE);
    if (NULL == event)
    {
        PLOG(ERROR)<<"add new connection fd:"<<sock<<" failed";
        return -1;
    }

    // TODO setwatermark
    bufferevent_enable(event, EV_READ|EV_WRITE);
    bufferevent_setcb(event, on_frontend_data_cb_func, NULL,
                      on_nondata_event_cb_func, this);

    frontend_handler_mgr_.AddHandler(event);

    return 0;
}

int AppLauncher::OnFrontEndMsg(struct bufferevent* bev)
{
    assert(bev != NULL);
    assert(app_ != NULL);

    // get msg from bufferevent
    size_t msg_size = bufferevent_read(bev, frontend_handler_mgr_.recv_buffer_.buffer,
                                       frontend_handler_mgr_.recv_buffer_.size);

    LOG(INFO)<<"recv total "<<msg_size<<" bytes from frontend";
    app_->OnRecvFrontEnd(frontend_handler_mgr_.recv_buffer_.buffer,
                         msg_size);

    return 0;
}

int AppLauncher::OnFrontEndSocketEvent(struct bufferevent* bev, short events)
{
    assert(bev != NULL);

    PLOG(INFO)<<"recv socket event:"<<events;

    // frontend event
    if (events & BEV_EVENT_ERROR)
    {
        PLOG(ERROR)<<"socket error";
        frontend_handler_mgr_.RemoveHandler(bev);
        bufferevent_free(bev);
        return 0;
    }

    if (events & BEV_EVENT_TIMEOUT)
    {
        PLOG(INFO)<<"socket read/write timeout";
        return 0;
    }

    if (events & BEV_EVENT_EOF)
    {
        PLOG(INFO)<<"peer close connection actively";
        frontend_handler_mgr_.RemoveHandler(bev);
        bufferevent_free(bev);
        return 0;
    }

    return 0;
}

int AppLauncher::ParseCmdLine(int argc, char** argv)
{
    assert(argc > 0 && argv != NULL);

    gflags::SetVersionString(app_->GetAppVersion());
    gflags::SetUsageMessage("server application based on rapidapp");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // listen uri
    if (FLAGS_uri.length() >= sizeof(setting_.listen_uri))
    {
        fprintf(stderr, "uri(length:%u) is too long\n", (unsigned)FLAGS_uri.length());
        return -1;
    }
    strcpy(setting_.listen_uri, FLAGS_uri.c_str());

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
