#include "rapidapp_framework_imp.h"
#include "utils/rap_net_uri.h"
#include <glog/logging.h>
#include <cassert>
#include <cstring>

DEFINE_string(uri, "tcp:0.0.0.0:80", "server listen uri, format: [proto:ip:port]");
DEFINE_string(log_file, "", "logging file name");
DEFINE_int32(fps, 1000, "frame-per-second, MUST >= 1, associated with reload and timer");

namespace rapidapp {
const int MAX_TCP_BACKLOG = 102400;
const char* udp_uri = "udp:127.0.0.1:9090";

bool AppFrameWork::running_ = false;
bool AppFrameWork::reloading_ = false;

void libevent_log_cb_func(int severity, const char *msg) {
    if (msg != NULL) {
        PLOG(INFO)<<"libevent log:"<<msg;
    }
}

AppFrameWork::AppFrameWork() : event_base_(NULL), internal_timer_(NULL),
                                 listener_(NULL), udp_ctrl_keeper_(NULL), ctrl_dispatcher_(),
                                 app_(NULL), frontend_handler_mgr_(), backend_handler_mgr_()
{
    memset(&setting_, 0, sizeof(setting_));
}

AppFrameWork::~AppFrameWork()
{
    //CleanUp();
}

void AppFrameWork::InitSignalHandle()
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

int AppFrameWork::ParseCmdLine(int argc, char** argv)
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

int AppFrameWork::InitLogging(int argc, char** argv)
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

int AppFrameWork::Init(RapidApp* app, int argc, char** argv)
{
    if (NULL == app)
    {
        return -1;
    }

    app_ = app;

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

    size_t size = 2 * app_->GetFrontEndMaxMsgSize();
    ret = frontend_handler_mgr_.Init(size);
    if (ret != 0)
    {
        LOG(ERROR)<<"init frontend net mgr failed";
        return -1;
    }

    size = 2 * app_->GetBackEndMaxMsgSize();
    ret = backend_handler_mgr_.Init(size);
    if (ret != 0)
    {
        LOG(ERROR)<<"init backend net mgr failed";
        return -1;
    }

    return 0;
}

int AppFrameWork::CleanUp()
{
    backend_handler_mgr_.CleanUp();
    frontend_handler_mgr_.CleanUp();

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

    google::ShutdownGoogleLogging();
    gflags::ShutDownCommandLineFlags();

    return 0;
}

void AppFrameWork::MainLoop()
{
#ifdef _DEBUG
    assert(event_base_ != NULL);
    event_base_dump_events(event_base_, stdout);
#endif

    // 3. mainloop
    running_ = true;
    event_base_dispatch(event_base_);
}

int AppFrameWork::Tick()
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

int AppFrameWork::Reload()
{
    assert(app_ != NULL);
    assert(event_base_ != NULL);

    return 0;
}

int AppFrameWork::OnCtrlMsg(struct bufferevent* bev)
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

int AppFrameWork::OnFrontEndConnect(evutil_socket_t sock, struct sockaddr *addr)
{
    assert(sock >= 0 && addr != NULL);

    PLOG(INFO)<<"has accepted new connect:"<<
        inet_ntoa(((struct sockaddr_in*)addr)->sin_addr);


    EasyNet* easy_net_handler = frontend_handler_mgr_.AddHandlerBySocket(sock, event_base_);
    if (NULL == easy_net_handler)
    {
        LOG(ERROR)<<"AddHandlerBySocket failed, socket:"<<sock;
        return -1;
    }

    bufferevent_setcb(easy_net_handler->hevent_, on_frontend_data_cb_func, NULL,
                      on_frontend_nondata_event_cb_func, this);
    // TODO setwatermark

    return 0;
}

int AppFrameWork::OnFrontEndMsg(struct bufferevent* bev)
{
    assert(bev != NULL);
    assert(app_ != NULL);

    // get msg from bufferevent
    size_t msg_size = bufferevent_read(bev, frontend_handler_mgr_.recv_buffer_.buffer,
                                       frontend_handler_mgr_.recv_buffer_.size);

    LOG(INFO)<<"recv total "<<msg_size<<" bytes from frontend";

    EasyNet* easy_net = frontend_handler_mgr_.GetHandlerByEvent(bev);
    if (NULL == easy_net)
    {
        LOG(ERROR)<<"get handler by event failed";
        return -1;
    }

    app_->OnRecvFrontEnd(easy_net,
                         frontend_handler_mgr_.recv_buffer_.buffer,
                         msg_size);

    return 0;
}

int AppFrameWork::OnFrontEndSocketEvent(struct bufferevent* bev, short events)
{
    assert(bev != NULL);

    PLOG(INFO)<<"recv socket event:"<<events;

    // frontend event
    if (events & BEV_EVENT_ERROR)
    {
        PLOG(ERROR)<<"socket error";
        frontend_handler_mgr_.RemoveHandlerByEvent(bev);
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
        frontend_handler_mgr_.RemoveHandlerByEvent(bev);
        return 0;
    }

    return 0;
}

int AppFrameWork::OnBackEndMsg(struct bufferevent* bev)
{
    assert(bev != NULL);
    assert(app_ != NULL);

    // get msg from bufferevent
    size_t msg_size = bufferevent_read(bev, backend_handler_mgr_.recv_buffer_.buffer,
                                       backend_handler_mgr_.recv_buffer_.size);

    LOG(INFO)<<"recv total "<<msg_size<<" bytes from backend";

    EasyNet* easy_net = backend_handler_mgr_.GetHandlerByEvent(bev);
    if (NULL == easy_net)
    {
        LOG(ERROR)<<"get handler by event failed";
        return -1;
    }

    app_->OnRecvBackEnd(easy_net,
                        backend_handler_mgr_.recv_buffer_.buffer,
                        msg_size);

    return 0;
}

int AppFrameWork::OnBackEndSocketEvent(struct bufferevent* bev, short events)
{
    assert(bev != NULL);

    PLOG(INFO)<<"recv socket event:"<<events;

    // backend event
    if (events & BEV_EVENT_ERROR)
    {
        PLOG(ERROR)<<"socket error";
        backend_handler_mgr_.RemoveHandlerByEvent(bev);
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
        backend_handler_mgr_.RemoveHandlerByEvent(bev);
        return 0;
    }

    return 0;
}

EasyNet* AppFrameWork::CreateBackEnd(const char* uri)
{
    EasyNet* easy_net_handler = backend_handler_mgr_.AddHandlerByUri(uri, event_base_);
    if (NULL == easy_net_handler)
    {
        LOG(ERROR)<<"AddHandlerByUri failed, uri:"<<uri;
        return NULL;
    }

    bufferevent_setcb(easy_net_handler->hevent_, on_backend_data_cb_func, NULL,
                      on_backend_nondata_event_cb_func, this);

    return easy_net_handler;
}

void AppFrameWork::DestroyBackEnd(EasyNet** net)
{
    if (NULL == net || NULL == *net)
    {
        LOG(WARNING)<<"invalid net";
        return;
    }

    backend_handler_mgr_.RemoveHandler((*net));

    *net = NULL;
}

int AppFrameWork::SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size)
{
    if (NULL == net || NULL == buf || 0 == buf_size)
    {
        LOG(ERROR)<<"null net || null buf || buf_size == 0";
        return -1;
    }

    if (net->Send(buf, buf_size) != 0)
    {
        LOG(ERROR)<<"send buf size:"<<buf_size<<" failed";
        return -1;
    }

    return 0;
}

int AppFrameWork::SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size)
{
    if (NULL == net || NULL == buf || 0 == buf_size)
    {
        LOG(ERROR)<<"null net || null buf || buf_size == 0";
        return -1;
    }

    if (net->Send(buf, buf_size) != 0)
    {
        LOG(ERROR)<<"send buf size:"<<buf_size<<" failed";
        return -1;
    }

    return 0;
}

}
