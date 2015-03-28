#include "rapidapp_framework_imp.h"
#include "rapidapp_easy_rpc.h"
#include "utils/rap_net_uri.h"
#include "utils/tcp_socket.h"
#include <event2/buffer.h>
#include <gflags/gflags.h>
#include <cassert>
#include <cstring>
#include <sys/resource.h>

#ifdef GFLAGS_NS_GOOGLE
#define GFLAGS_NS google
#else
#define GFLAGS_NS gflags
#endif

DEFINE_string(uri, "tcp:0.0.0.0:80", "server listen uri, format: [proto:ip:port]");
DEFINE_string(log_file, "", "logging file name");
DEFINE_int32(fps, 1000, "frame-per-second, MUST >= 1, associated with reload and timer");
DEFINE_int32(max_idle, -1, "max idle time on frontend, time unit s, default no idle limit");
DEFINE_int32(max_pkg_speed, -1, "max pkg number per frame on frontend, default no limit");
DEFINE_int32(max_traffic_speed, -1, "max traffic per frame on frontend, default no limit");

DEFINE_int32(max_cocurrent_rpc, 10240, "max cocurrent rpc number");
DEFINE_int32(rpc_stack_size, 1024, "rpc stack size to save user context");

DEFINE_string(pid_file, "", "pid file name, default {appname}.pid");
DEFINE_string(ctrl_file, "", "ctrl sock file name, default {appname}.sock");

DEFINE_bool(daemon, false, "run as deamon");
DEFINE_bool(stop, false, "stop the daemon according to pid file");
DEFINE_bool(restart, false, "restart the daemon according to pid file");
DEFINE_bool(reload, false, "trigger reload event according to pid file");

namespace rapidapp {
// global variable & function
const int MAX_TCP_BACKLOG = 102400;

#ifdef _DEBUG
#define UDP_HOST "127.0.0.1"
#else
#define UDP_HOST "0.0.0.0"
#endif
#define UDP_PORT_MIN 19090
#define UDP_PORT_MAX (UDP_PORT_MIN + 1024)
#define UDP_PORT_COUNT (UDP_PORT_MAX - UDP_PORT_MIN)

const int MAX_CTRL_MSG_LEN = 2048;

const int DEFAULT_MAX_FD_LIMIT = 10240;

const int MAX_ARGUMENTS_NUM = 32;

bool AppFrameWork::running_ = false;
bool AppFrameWork::reloading_ = false;
time_t AppFrameWork::now_ = 0;

const char CTRL_CMD_DELIMETER = ' ';

#define PID_SUFFIX ".pid"
const int PID_SUFFIX_LEN = sizeof(PID_SUFFIX);
const int MAX_PID_FILE_NAME_LEN = 1024;

#define SOCK_SUFFIX ".sock"
const int SOCK_SUFFIX_LEN = sizeof(SOCK_SUFFIX);
const int MAX_SOCK_FILE_NAME_LEN = 1024;

void libevent_log_cb_func(int severity, const char *msg) {
    if (msg != NULL) {
        PLOG(INFO)<<"libevent log:"<<msg;
    }
}

inline int libevent_get_arguments(char* command, int& argc, char** argv) {
    if (NULL == argv || NULL == command) {
        return -1;
    }

    int i = 0;
    char* start_ptr = command;
    char* ptr = NULL;
    while (strtok_r(start_ptr, const_cast<char*>(&CTRL_CMD_DELIMETER), &ptr) != NULL
           && i < MAX_ARGUMENTS_NUM) {
        argv[i++] = start_ptr;
        *ptr = '\0';
        start_ptr = ptr + 1;
    }
    argc = i;

    return 0;
}

AppFrameWork::AppFrameWork() : event_base_(NULL), internal_timer_(NULL), listener_(NULL),
                                 udp_ctrl_keeper_(NULL), ctrl_dispatcher_(),
                                 app_(NULL), schedule_update_(false), has_been_cleanup_(false),
                                 frontend_handler_mgr_(), backend_handler_mgr_(),
                                 timer_mgr_(), rpc_scheduler_(NULL), pid_fp_(NULL)
{
    memset(&setting_, 0, sizeof(setting_));
}

AppFrameWork::~AppFrameWork()
{
    if (!has_been_cleanup_)
    {
        CleanUp();
    }
}

void AppFrameWork::InitSignalHandle()
{
    signal(SIGHUP,  SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    struct sigaction sig_act;
    memset(&sig_act, 0, sizeof(sig_act));

    sig_act.sa_flags = SA_SIGINFO;
    sig_act.sa_sigaction = signal_stop_handler;
    sigaction(SIGTERM, &sig_act, NULL);
    sigaction(SIGINT, &sig_act, NULL);
    sigaction(SIGQUIT, &sig_act, NULL);
    sigaction(SIGUSR1, &sig_act, NULL);

    sig_act.sa_sigaction = signal_reload_handler;
    sigaction(SIGUSR2, &sig_act, NULL);
}

void AppFrameWork::MakeDaemon()
{
    pid_t pid = fork();
    if (pid != 0)
        exit(0);

    // first children
    if (-1 == setsid())
    {
        printf("pid:%d setsid failed\n", getpid());
        exit(-1);
    }

    // second children
    pid = fork();
    if (pid != 0)
        exit(0);

    int stdfd = open("/dev/null", O_RDWR);
    dup2(stdfd, STDOUT_FILENO);
    dup2(stdfd, STDERR_FILENO);
}

int AppFrameWork::ParseCmdLine(int argc, char** argv)
{
    assert(argc > 0 && argv != NULL);

    GFLAGS_NS::SetVersionString(app_->GetAppVersion());
    GFLAGS_NS::SetUsageMessage("server application based on rapidapp");
    GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

    // mode
    if (FLAGS_stop)
    {
        if (FLAGS_restart || FLAGS_reload)
        {
            fprintf(stderr, "only 1 mode supported simulately in [stop|restart|reload]");
            return -1;
        }
        setting_.mode = APP_STOP;
    }
    else if (FLAGS_restart && FLAGS_reload)
    {
        fprintf(stderr, "only 1 mode supported simulately in [stop|restart|reload]");
        return -1;
    }
    else if (FLAGS_restart)
    {
        setting_.mode = APP_RESTART;
    }
    else if (FLAGS_reload)
    {
        setting_.mode = APP_RELOAD;
    }
    else
    {
        setting_.mode = APP_START;
    }

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

    // pid file
    if (0 != FLAGS_pid_file.length())
    {
        if (FLAGS_pid_file.length() >= sizeof(setting_.pid_file_name))
        {
            fprintf(stderr, "pid_file(length:%u) is too long\n", (unsigned)FLAGS_pid_file.length());
            return -1;
        }
        strcpy(setting_.pid_file_name, FLAGS_pid_file.c_str());
    }
    else
    {
        if (strlen(argv[0]) + PID_SUFFIX_LEN >= sizeof(setting_.pid_file_name))
        {
            fprintf(stderr, "appname:%s too long\n", argv[0]);
            return -1;
        }
        snprintf(setting_.pid_file_name, sizeof(setting_.pid_file_name),
                 "%s"PID_SUFFIX, argv[0]);
    }

    if (0 != FLAGS_ctrl_file.length())
    {
        if (FLAGS_ctrl_file.length() >= sizeof(setting_.ctrl_sock_file_name))
        {
            fprintf(stderr, "ctrl_file(length:%u) is too long\n", (unsigned)FLAGS_ctrl_file.length());
            return -1;
        }
        strcpy(setting_.ctrl_sock_file_name, FLAGS_ctrl_file.c_str());
    }
    else
    {
        if (strlen(argv[0]) + SOCK_SUFFIX_LEN >= sizeof(setting_.ctrl_sock_file_name))
        {
            fprintf(stderr, "appname:%s too long\n", argv[0]);
            return -1;
        }
        snprintf(setting_.ctrl_sock_file_name, sizeof(setting_.ctrl_sock_file_name),
                 "%s"SOCK_SUFFIX, argv[0]);
    }

    // fps
    if (FLAGS_fps < 1)
    {
        fprintf(stderr, "fps MUST >= 1");
        return -1;
    }
    setting_.fps = FLAGS_fps;

    // max idle
    setting_.max_idle = FLAGS_max_idle;
    setting_.max_pkg_speed = FLAGS_max_pkg_speed;
    setting_.max_traffic_speed = FLAGS_max_traffic_speed;

    return 0;
}

int AppFrameWork::SetResourceLimit(int fd_limit)
{
    if (fd_limit <= 0)
    {
        LOG(ERROR)<<"bad fd_limit"<<fd_limit;
        return -1;
    }

    struct rlimit rlmt;
    if (getrlimit(RLIMIT_NOFILE, &rlmt) != 0)
    {
        PLOG(ERROR)<<"getrlimit failed";
        return -1;
    }

    if ((int)rlmt.rlim_cur < fd_limit)
    {
        LOG(INFO)<<"current system resoure limit:"<<rlmt.rlim_cur<<
            " less than expected:"<<fd_limit<<", so setrlimit";
        rlmt.rlim_cur = fd_limit;
        rlmt.rlim_max = fd_limit;
        if (setrlimit(RLIMIT_NOFILE, &rlmt) != 0)
        {
            PLOG(ERROR)<<"setrlimit to "<<fd_limit<<" failed";
            return -1;
        }
    }

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

// 进程退出时会自动unlock pidfile，所以不会主动去调用unlock
int AppFrameWork::SetPidFile()
{
    // mode 'w+' will definitely truncate pid file, must set mode 'a'
    pid_fp_ = fopen(setting_.pid_file_name, "a");
    if (NULL == pid_fp_)
    {
        fprintf(stderr, "fopen file:%s failed\n", setting_.pid_file_name);
        return -1;
    }

    int ret = fcntl(fileno(pid_fp_), F_SETFD, FD_CLOEXEC);
    if (-1 == ret)
    {
        fprintf(stderr, "set file:%s CLOEXEC failed\n", setting_.pid_file_name);
        return -1;
    }

    static struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    lock.l_pid = getpid();
    ret = fcntl(fileno(pid_fp_), F_SETLK, &lock);
    if (-1 == ret)
    {
        fprintf(stderr, "try lock file:%s failed\n", setting_.pid_file_name);
        return -1;
    }

    ftruncate(fileno(pid_fp_), 0);
    char pid[MAX_PID_FILE_NAME_LEN];
    snprintf(pid, sizeof(pid), "%d\n", getpid());
    fputs(pid, pid_fp_);
    fflush(pid_fp_);

    return 0;
}

int AppFrameWork::GetRunningPid()
{
    pid_fp_ = fopen(setting_.pid_file_name, "r+");
    if (NULL == pid_fp_)
    {
        fprintf(stderr, "fopen file:%s failed\n", setting_.pid_file_name);
        return -1;
    }

    int ret = fcntl(fileno(pid_fp_), F_SETFD, FD_CLOEXEC);
    if (-1 == ret)
    {
        fprintf(stderr, "set file:%s CLOEXEC failed\n", setting_.pid_file_name);
        return -1;
    }

    static struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;
    lock.l_pid = getpid();
    ret = fcntl(fileno(pid_fp_), F_SETLK, &lock);
    if (-1 != ret)
    {
        return 0;
    }

    // pid file has been held by another process
    char pid[MAX_PID_FILE_NAME_LEN];
    if (NULL == fgets(pid, sizeof(pid), pid_fp_))
    {
        fprintf(stderr, "fgets file:%s failed\n", setting_.pid_file_name);
        return -1;
    }

    return strtol(pid, NULL, 0);
}

int AppFrameWork::SetCtrlSockFile()
{
    // 1. open udp sock
    int hash = BKDRHash(setting_.ctrl_sock_file_name);
    int port = UDP_PORT_MIN + (hash % UDP_PORT_COUNT);
    evutil_socket_t udp_ctrl_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_ctrl_sockfd < 0)
    {
        PLOG(ERROR)<<"open udp socket failed";
        return -1;
    }

    struct sockaddr_in udp_sock_addr;
    udp_sock_addr.sin_family = AF_INET;
    udp_sock_addr.sin_port = htons(port);
    udp_sock_addr.sin_addr.s_addr = inet_addr(UDP_HOST);
    if (-1 == bind(udp_ctrl_sockfd,
                   (struct sockaddr *)&udp_sock_addr, sizeof(udp_sock_addr)))
    {
        PLOG(ERROR)<<"bind udp host:"<<UDP_HOST<<", port:"<<port<<" failed";
        return -1;
    }

    evutil_make_socket_nonblocking(udp_ctrl_sockfd);
    evutil_make_socket_closeonexec(udp_ctrl_sockfd);
    int opt = 0;
    setsockopt(udp_ctrl_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    udp_ctrl_keeper_ = bufferevent_socket_new(event_base_, udp_ctrl_sockfd,
                                              BEV_OPT_CLOSE_ON_FREE);
    if (NULL == udp_ctrl_keeper_)
    {
        PLOG(ERROR)<<"create bufferevent for ctrl keeper failed";
        return -1;
    }
    bufferevent_enable(udp_ctrl_keeper_, EV_READ|EV_WRITE);
    bufferevent_setcb(udp_ctrl_keeper_, on_ctrl_cb_func, NULL, NULL, this);

    // 2. write .sock file
    FILE* fp = fopen(setting_.ctrl_sock_file_name, "w+");
    if (NULL == fp)
    {
        fprintf(stderr, "fopen file:%s failed\n", setting_.pid_file_name);
        return -1;
    }

    int ret = fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
    if (-1 == ret)
    {
        fprintf(stderr, "set file:%s CLOEXEC failed\n", setting_.pid_file_name);
        return -1;
    }

    fprintf(fp, "%s %d\n", UDP_HOST, port);
    fflush(fp);

    return 0;
}

int AppFrameWork::InitNormalMode(RapidApp* app, int argc, char** argv)
{
    int ret = 0;

    // run as daemon
    if (FLAGS_daemon)
    {
        MakeDaemon();
    }

    // glog
    ret = InitLogging(argc, argv);
    if (ret != 0)
    {
        fprintf(stderr, "Init logger failed");
        return -1;
    }

    // set pid file
    ret = SetPidFile();
    if (ret != 0)
    {
        fprintf(stderr, "SetPidFile failed\n");
        return -1;
    }

    // signal
    InitSignalHandle();

#ifndef _DEBUG
    // 设置系统资源限制
    ret = SetResourceLimit(DEFAULT_MAX_FD_LIMIT);
    if (ret != 0)
    {
        LOG(ERROR)<<stderr, "set resource limit failed";
        return -1;
    }
#endif

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
    // 添加内部支持命令字
    ctrl_dispatcher_.AddDefaultSupportedCommand();
    ret = SetCtrlSockFile();
    if (ret != 0)
    {
        LOG(ERROR)<<"set ctrl sock file:"<<setting_.ctrl_sock_file_name<<" failed";
        return -1;
    }

    // tcp listener
    struct sockaddr_in listen_sa;
    ret = tcpsocket_str2sockin(setting_.listen_uri, &listen_sa);
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

    ret = timer_mgr_.Init(DEFAULT_MAX_TIMER_NUM);
    if (ret != 0)
    {
        LOG(ERROR)<<"init timer mgr failed";
        return -1;
    }

    rpc_scheduler_ = new(std::nothrow) magic_cube::CoroutineScheduler(
                                                    FLAGS_max_cocurrent_rpc,
                                                    FLAGS_rpc_stack_size);
    if (NULL == rpc_scheduler_)
    {
        LOG(ERROR)<<"init rpc scheduler failed";
        return -1;
    }

    now_ = time(NULL);
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

    // 如果stop命令，通过发SIGUSR1信号停掉运行中进程
    switch (setting_.mode)
    {
        case APP_STOP:
            {
                pid_t running_pid = GetRunningPid();
                if (-1 == running_pid)
                {
                    exit(-1);
                }
                else
                {
                    if (running_pid != 0)
                    {
                        kill(running_pid, SIGUSR1);
                    }

                    exit(0);
                }

                break;
            }
        case APP_RESTART:
            {
                pid_t running_pid = GetRunningPid();
                if (-1 == running_pid)
                {
                    exit(-1);
                }
                else if (running_pid != 0)
                {
                    kill(running_pid, SIGUSR1);
                }

                break;
            }
        case APP_RELOAD:
            {
                pid_t running_pid = GetRunningPid();
                if (-1 == running_pid)
                {
                    exit(-1);
                }
                else
                {
                    if (running_pid != 0)
                    {
                        kill(running_pid, SIGUSR2);
                    }

                    exit(0);
                }

                break;
            }
        default:
            {
                break;
            }
    }

    return InitNormalMode(app, argc, argv);
}

int AppFrameWork::CleanUp()
{
    if (rpc_scheduler_ != NULL)
    {
        delete rpc_scheduler_;
        rpc_scheduler_ = NULL;
    }

    timer_mgr_.CleanUp();
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
    GFLAGS_NS::ShutDownCommandLineFlags();

    has_been_cleanup_ = true;

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

    now_ = time(NULL);

    if (!running_)
    {
        LOG(INFO)<<"exit loop";
        event_base_loopbreak(event_base_);
    }

    if (reloading_)
    {
        Reload();
        reloading_ = false;
    }

    if (schedule_update_)
    {
        app_->OnUpdate();
    }

    // TODO system max CPU overload, getloadavg
    // TODO system max memcory useage, /proc/$pid/meminfo

    // 检查定时器事件是否触发
    while (true)
    {
        EasyTimer* timer = timer_mgr_.GetNextActiveTimer();

        if (NULL == timer)
        {
            break;
        }
        else
        {
            app_->OnTimer(timer, timer->timer_id());
        }
    }

    // 空闲连接检查
    // 上行包速检查
    if (setting_.max_idle > 0 || setting_.max_pkg_speed > 0 ||
        setting_.max_traffic_speed > 0)
    {
        frontend_handler_mgr_.WalkThrough(this);
    }

    return 0;
}

int AppFrameWork::Reload()
{
    assert(app_ != NULL);
    assert(event_base_ != NULL);

    LOG(INFO)<<"app begin reload<<<";
    app_->OnReload();
    LOG(INFO)<<">>>app end reload";

    return 0;
}

// string command supported only
int AppFrameWork::OnCtrlMsg(struct bufferevent* bev)
{
    assert(app_ != NULL);
    assert(event_base_ != NULL);

    if (NULL == bev)
    {
        PLOG(ERROR)<<"bev is null, but event triggered";
        return -1;
    }

    // 控制模式采用UDP通信，不存在分包情况
    static char ctrl_msg[MAX_CTRL_MSG_LEN];
    size_t len = bufferevent_read(bev, ctrl_msg, sizeof(ctrl_msg));
    if (0 == len)
    {
        PLOG(ERROR)<<"null command";
        return -1;
    }
    ctrl_msg[MAX_CTRL_MSG_LEN - 1] = '\0';

    LOG(INFO)<<"recv ctrl msg:"<<ctrl_msg;

    int argc = 0;
    char* argv[MAX_ARGUMENTS_NUM];
    int ret = libevent_get_arguments(ctrl_msg, argc, argv);
    if (ret != 0)
    {
        LOG(ERROR)<<"get arguments:"<<ctrl_msg<<" failed";
        return -1;
    }

    ret = ctrl_dispatcher_.Dispatch(argc, argv);
    if (ret != 0)
    {
        // non-rapidapp inner control command, user specified control msg
        ret = app_->OnRecvCtrl(argc, argv);
        if (ret < 0)
        {
            LOG(ERROR)<<"invalid control command:"<<ctrl_msg;
        }
    }

    return 0;
}

int AppFrameWork::OnFrontEndConnect(evutil_socket_t sock, struct sockaddr *addr)
{
    assert(sock >= 0 && addr != NULL && app_ != NULL);

    char* ip = inet_ntoa(((struct sockaddr_in*)addr)->sin_addr);
    unsigned short port = ntohs(((struct sockaddr_in*)addr)->sin_port);
    LOG(INFO)<<"has accepted new tcp connect:"<<ip<<":"<<port;

    EasyNet* easy_net_handler = frontend_handler_mgr_.AddHandlerBySocket
        (sock, 0/*前端服务目前不区分类型*/, event_base_);
    if (NULL == easy_net_handler)
    {
        LOG(ERROR)<<"AddHandlerBySocket failed, socket:"<<sock;
        return -1;
    }

    int len = snprintf(easy_net_handler->uri(), MAX_URL_LEN - 1,
                       "%s:%d", ip, (int)port);
    if (len < 0 || len >= MAX_URL_LEN)
    {
        LOG(ERROR)<<"format frontend uri failed";
        return -1;
    }

    bufferevent_setcb(easy_net_handler->hevent_, on_frontend_data_cb_func, NULL,
                      on_frontend_nondata_event_cb_func, this);

    size_t uctx_size = app_->GetFrontEndContextSize();
    if (uctx_size > 0)
    {
        if (easy_net_handler->CreateUserContext(uctx_size) != 0)
        {
            PLOG(ERROR)<<"create context size:"<<uctx_size;
            frontend_handler_mgr_.RemoveHandler(easy_net_handler);
            return -1;
        }
    }

    easy_net_handler->set_active_time(now_);

    // TODO record max connector number
    // TODO setwatermark

    app_->OnFrontEndConnect(easy_net_handler, easy_net_handler->net_type());

    return 0;
}

int AppFrameWork::OnFrontEndMsg(struct bufferevent* bev)
{
    assert(bev != NULL);
    assert(app_ != NULL);

    // get msg from bufferevent
    //size_t msg_size = bufferevent_read(bev, frontend_handler_mgr_.recv_buffer_.buffer,
                                       //frontend_handler_mgr_.recv_buffer_.size);

    struct evbuffer* evbuf = bufferevent_get_input(bev);
    if (NULL == evbuf)
    {
        LOG(ERROR)<<"assert failed, evbuf of frontend is NULL";
        return -1;
    }

    ev_ssize_t msg_size = evbuffer_copyout(evbuf, frontend_handler_mgr_.recv_buffer_.buffer,
                                           frontend_handler_mgr_.recv_buffer_.size);
    if (-1 == msg_size)
    {
        LOG(ERROR)<<"could not drain from buffer";
        return -1;
    }

    LOG(INFO)<<"recv total "<<msg_size<<" bytes from frontend";

    EasyNet* easy_net = frontend_handler_mgr_.GetHandlerByEvent(bev);
    if (NULL == easy_net)
    {
        LOG(ERROR)<<"get handler by event failed";
        return -1;
    }

    size_t elapsed_msglen = 0;
    while(msg_size > elapsed_msglen)
    {
        size_t msglen = GetMsgLength(
                        frontend_handler_mgr_.recv_buffer_.buffer + elapsed_msglen,
                        msg_size - elapsed_msglen);
        if (0 == msglen || msglen > (msg_size - elapsed_msglen))
        {
            LOG(INFO)<<"recved left msg size:"<<msg_size - elapsed_msglen
                <<", but msg len:"<<msglen<<"---NOT complete yet";
            break;
        }

        LOG(INFO)<<"current msg len:"<<msglen - sizeof(uint32_t);

        app_->OnRecvFrontEnd(easy_net, easy_net->net_type(),
                             frontend_handler_mgr_.recv_buffer_.buffer\
                             + elapsed_msglen + sizeof(uint32_t),
                             msglen - sizeof(uint32_t));
        ++(easy_net->curr_pkg_count_);
        elapsed_msglen += msglen;
    }
    evbuffer_drain(evbuf, elapsed_msglen);

    easy_net->curr_traffic_bytes_ += elapsed_msglen;
    easy_net->set_active_time(now_);

    return 0;
}

int AppFrameWork::OnFrontEndSocketEvent(struct bufferevent* bev, short events)
{
    assert(bev != NULL);
    assert(app_ != NULL);

    PLOG(INFO)<<"recv socket event:"<<events;

    // frontend event
    if (events & BEV_EVENT_ERROR)
    {
        PLOG(ERROR)<<"socket error";
        EasyNet* net = frontend_handler_mgr_.GetHandlerByEvent(bev);
        if (net != NULL)
        {
            app_->OnFrontEndClose(net, net->net_type());
        }

        frontend_handler_mgr_.RemoveHandlerByEvent(bev);
        return 0;
    }

    if (events & BEV_EVENT_TIMEOUT)
    {
        PLOG(INFO)<<"socket "<<bufferevent_getfd(bev)<<" read/write timeout";
        return 0;
    }

    if (events & BEV_EVENT_EOF)
    {
        PLOG(INFO)<<"peer close connection actively";
        EasyNet* net = frontend_handler_mgr_.GetHandlerByEvent(bev);
        if (net != NULL)
        {
            app_->OnFrontEndClose(net, net->net_type());
        }

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
    //size_t msg_size = bufferevent_read(bev, backend_handler_mgr_.recv_buffer_.buffer,
                                       //backend_handler_mgr_.recv_buffer_.size);

    struct evbuffer* evbuf = bufferevent_get_input(bev);
    if (NULL == evbuf)
    {
        LOG(ERROR)<<"assert failed, evbuf of backend input is NULL";
        return -1;
    }

    ev_ssize_t msg_size = evbuffer_copyout(evbuf, backend_handler_mgr_.recv_buffer_.buffer,
                                           backend_handler_mgr_.recv_buffer_.size);
    if (-1 == msg_size)
    {
        LOG(ERROR)<<"could not drain from buffer";
        return -1;
    }

    LOG(INFO)<<"recv total "<<msg_size<<" bytes from backend";

    EasyNet* easy_net = backend_handler_mgr_.GetHandlerByEvent(bev);
    if (NULL == easy_net)
    {
        LOG(ERROR)<<"get handler by event failed";
        return -1;
    }

    size_t elapsed_msglen = 0;
    while(msg_size > elapsed_msglen)
    {
        size_t msglen = GetMsgLength(
                        backend_handler_mgr_.recv_buffer_.buffer + elapsed_msglen,
                        msg_size - elapsed_msglen);
        if (0 == msglen || msglen > (msg_size - elapsed_msglen))
        {
            LOG(INFO)<<"recved left msg size:"<<msg_size - elapsed_msglen
                <<", but msg len:"<<msglen<<"---NOT complete yet";
            break;
        }

        LOG(INFO)<<"current msg len:"<<msglen - sizeof(uint32_t);

        EasyRpc* rpc = static_cast<EasyRpc*>(easy_net->rpc_binded());
        if (easy_net->is_rpc_binded() && rpc != NULL && rpc->IsActive())
        {
            rpc->Resume(backend_handler_mgr_.recv_buffer_.buffer \
                        + elapsed_msglen + sizeof(uint32_t),
                        msglen - sizeof(uint32_t));
        }
        else
        {
            app_->OnRecvBackEnd(easy_net, easy_net->net_type(),
                                backend_handler_mgr_.recv_buffer_.buffer \
                                + elapsed_msglen + sizeof(uint32_t),
                                msglen - sizeof(uint32_t));
        }
        elapsed_msglen += msglen;
    }
    evbuffer_drain(evbuf, elapsed_msglen);

    return 0;
}

int AppFrameWork::OnBackEndSocketEvent(struct bufferevent* bev, short events)
{
    assert(bev != NULL);

    LOG(INFO)<<"recv socket event:"<<events<<", fd:"<<bufferevent_getfd(bev);

    // backend event
    if (events & BEV_EVENT_ERROR)
    {
        PLOG(ERROR)<<"socket error, fd:"<<bufferevent_getfd(bev);
        backend_handler_mgr_.ChangeNetStateByEvent(bev, NET_FAILED);
        return 0;
    }

    // libevent client模式（bufferevent_socket_connect）在收到BEV_EVENT_ERROR后
    // 此时收到TCP RST包，还会收到一次BEV_EVENT_CONNECTED事件，应该是个bug
    if (events & BEV_EVENT_CONNECTED)
    {
        LOG(INFO)<<"connect fd:"<<bufferevent_getfd(bev)<<" has been established";
        backend_handler_mgr_.ChangeNetStateByEvent(bev, NET_ESTABLISHED);
        return 0;
    }

    if (events & BEV_EVENT_TIMEOUT)
    {
        LOG(INFO)<<"socket read/write timeout";
        return 0;
    }

    if (events & BEV_EVENT_EOF)
    {
        LOG(INFO)<<"peer close connection actively";
        backend_handler_mgr_.ChangeNetStateByEvent(bev,NET_BEEN_CLOSED);
        return 0;
    }

    return 0;
}

EasyNet* AppFrameWork::CreateBackEnd(const char* uri, int type)
{
    EasyNet* easy_net_handler = backend_handler_mgr_.AddHandlerByUri(uri, type, event_base_);
    if (NULL == easy_net_handler)
    {
        LOG(ERROR)<<"AddHandlerByUri failed, uri:"<<uri<<", type:"<<type;
        return NULL;
    }

    bufferevent_setcb(easy_net_handler->hevent_, on_backend_data_cb_func, NULL,
                      on_backend_nondata_event_cb_func, this);

    size_t uctx_size = app_->GetBackEndContextSize();
    if (uctx_size > 0)
    {
        if (easy_net_handler->CreateUserContext(uctx_size) != 0)
        {
            PLOG(ERROR)<<"create backend net context size:"<<uctx_size;
            DestroyBackEnd(&easy_net_handler);
        }
    }
    easy_net_handler->set_active_time(now_);

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

void AppFrameWork::DestroyFrontEnd(EasyNet** net)
{
    if (NULL == net || NULL == *net)
    {
        LOG(WARNING)<<"invalid net";
        return;
    }

    frontend_handler_mgr_.RemoveHandler((*net));

    *net = NULL;
}

int AppFrameWork::SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size)
{
    if (NULL == net || NULL == buf || 0 == buf_size)
    {
        LOG(ERROR)<<"null net || null buf || buf_size == 0";
        return -1;
    }

    int ret = net->Send(buf, buf_size);
    if (ret != EASY_NET_OK)
    {
        LOG(ERROR)<<"send buf size:"<<buf_size<<" to frontend failed";
        return -1;
    }

    LOG(INFO)<<"send buf size:"<<buf_size<<" to frontend success";

    return 0;
}

int AppFrameWork::SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size)
{
    if (NULL == net || NULL == buf || 0 == buf_size)
    {
        LOG(ERROR)<<"null net || null buf || buf_size == 0";
        return -1;
    }

    int ret = net->Send(buf, buf_size);
    if (ret != EASY_NET_OK)
    {
        LOG(ERROR)<<"send buf size:"<<buf_size<<" to backend failed";
    }

    if (EASY_NET_ERR_NOT_YET_ESTABLISHED == ret)
    {
        return NET_NOT_ESTABLISHED;
    }
    else if (EASY_NET_ERR_ESTABLISH_FAILED == ret)
    {
        return NET_CONNECT_FAILED;
    }
    else if (EASY_NET_ERR_SEND_ERROR == ret)
    {
        return NET_SEND_EXCEPTION;
    }

    net->set_active_time(now_);
    LOG(INFO)<<"send buf size:"<<buf_size<<" to backend success";

    return 0;
}

EasyNet* AppFrameWork::GetFrontEndByAsyncIds(uint32_t fd, uint64_t nid)
{
    EasyNet* net = frontend_handler_mgr_.GetHandlerByAsyncIds(fd, nid);
    if (NULL == net)
    {
        return NULL;
    }

    return net;
}

int AppFrameWork::GetNetIds(EasyNet* net, uint32_t& fd, uint64_t& nid)
{
    if (NULL == net)
    {
        LOG(ERROR)<<"net is null";
        return -1;
    }

    fd = bufferevent_getfd(net->bufferevent());
    nid = net->nid();

    return 0;
}

void* AppFrameWork::GetUserContext(EasyNet* net)
{
    if (NULL == net)
    {
        return NULL;
    }

    return net->user_context();
}

EasyTimer* AppFrameWork::CreateTimer(size_t time, int timer_id)
{
    EasyTimer* timer = timer_mgr_.AddTimer(time, timer_id);
    if (NULL == timer)
    {
        LOG(ERROR)<<"AddTimer failed, time:"<<time<<", timer_id:"<<timer_id;
    }

    return timer;
}

void AppFrameWork::DestroyTimer(EasyTimer** timer)
{
    if (NULL == timer || NULL == *timer)
    {
        LOG(WARNING)<<"invalid timer";
        return;
    }

    timer_mgr_.RemoveTimer((*timer));

    *timer = NULL;
}

EasyRpc* AppFrameWork::CreateRpc(EasyNet* net)
{
    EasyRpc* rpc = new(std::nothrow) EasyRpc();
    if (NULL == rpc)
    {
        return NULL;
    }

    int ret = rpc->Init(rpc_scheduler_, net);
    if (ret != 0)
    {
        delete rpc;
        return NULL;
    }

    // 1个连接关联1个rpc实例，1个rpc实例可以同时发起多个rpc调用
    net->set_rpc_binded(rpc);

    return rpc;
}

int AppFrameWork::DestroyRpc(EasyRpc** rpc)
{
    if (NULL == rpc || NULL == *rpc)
    {
        return -1;
    }

    delete *rpc;
    *rpc = NULL;

    return 0;
}

int AppFrameWork::RpcCall(EasyRpc* rpc, const ::google::protobuf::Message* request,
                          ON_RPC_REPLY_FUNCTION callback)
{
    if (NULL == rpc || NULL == callback || NULL == request)
    {
        LOG(ERROR)<<"invalid rpc call arguments";
        return -1;
    }

    return rpc->RpcCall(request, callback);
}

void AppFrameWork::ScheduleUpdate()
{
    schedule_update_ = true;
}

// 前端net遍历操作
// 返回值!0 遍历后删除，0 遍历不删除
int AppFrameWork::DoSomething(EasyNet* net)
{
    if (setting_.max_idle > 0 && net != NULL)
    {
        if (now_ - net->last_active_time() > setting_.max_idle)
        {
            // erase
            LOG(WARNING)<<"net:"<<net->uri()<<" has been idle for: "<<now_ - net->last_active_time()
                <<" s, bigger than max uplimit: "<<setting_.max_idle<<" s, close it";
            return 1;
        }
    }

    if (setting_.max_pkg_speed > 0 && net != NULL)
    {
        if (net->curr_pkg_count_ - net->prev_pkg_count_ > setting_.max_pkg_speed)
        {
            // erase
            LOG(WARNING)<<"net:"<<net->uri()<<" has sent "
                <<net->curr_pkg_count_ - net->prev_pkg_count_
                <<" pkgs during 1 frame, bigger than max uplimit: "
                <<setting_.max_pkg_speed<<", close it";
            return 1;
        }
        else
        {
            net->prev_pkg_count_ = 0;
            net->curr_pkg_count_ = 0;
        }
    }

    if (setting_.max_traffic_speed > 0 && net != NULL)
    {
        if (net->curr_traffic_bytes_ - net->prev_traffic_bytes_ > setting_.max_traffic_speed)
        {
            // erase
            LOG(WARNING)<<"net:"<<net->uri()<<" has sent "
                <<net->curr_traffic_bytes_ - net->prev_traffic_bytes_
                <<" bytes during 1 frame, bigger than max uplimit: "
                <<setting_.max_traffic_speed<<", close it";
            return 1;
        }
        else
        {
            net->prev_traffic_bytes_ = 0;
            net->curr_traffic_bytes_ = 0;
        }
    }

    return 0;
}


} // namespace rapidapp
