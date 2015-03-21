/*
 * connector接入服务器。
 *
 * connector主要功能：
 * 1. 连接管理、排队
 * 2. 广播
 * 3. 串行汇聚
 * 4. 过载保护(先手工配置，再智能感知过载阀值)
 * 5. 断线重连
 * 6. traffic control(max upsize)
 * 7. 无状态，方便平行扩展
 * 8. 黑白名单机制
 * 9. 鉴权、安全通信通道
 *10. 监控上报
 * */
#include "connector_app.h"
#include "server.pb.h"
#include "client.pb.h"
#include <gflags/gflags.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <sstream>
#include <fstream>
#include <iostream>

DEFINE_string(config_file, "config.ini", "config file path name");

ConnectorApp::ConnectorApp() : frame_stub_(NULL)
{}

ConnectorApp::~ConnectorApp()
{}

int ConnectorApp::SetUpAndCheckConfig()
{
    LOG(INFO)<<"initialize from conf file: "<<FLAGS_config_file;

    FILE* fp = fopen(FLAGS_config_file.c_str(), "r");
    if (NULL == fp)
    {
        PLOG(ERROR)<<"open file: "<<FLAGS_config_file<<" failed";
        return -1;
    }

    if (0 != fseek(fp, 0, SEEK_END))
    {
        PLOG(ERROR)<<"fseek failed";
        return -1;
    }

    long filesize = ftell(fp);
    char* conf = static_cast<char*>(calloc(1, filesize));
    if (NULL == conf)
    {
        LOG(ERROR)<<"calloc size: "<<filesize<<" failed";
        fclose(fp);
        return -1;
    }

    rewind(fp);
    fread(conf, filesize, 1, fp);
    if (ferror(fp))
    {
        LOG(ERROR)<<"fgets failed";
        free(conf);
        fclose(fp);
        return -1;
    }

    std::string conf_str(conf, filesize);
    free(conf);
    fclose(fp);

    ::google::protobuf::TextFormat::ParseFromString(conf_str, &config_);
    LOG(INFO)<<"config:"<<std::endl<<config_.DebugString();

    if (config_.backends_size() <= 0)
    {
        LOG(ERROR)<<"need at least 1 backend";
        return -1;
    }

    return 0;
}

int ConnectorApp::OnInit(IFrameWork* app_framework)
{
    if (NULL == app_framework)
    {
        LOG(ERROR)<<"null app framework, assert failed";
        return -1;
    }

    // 保存框架存根
    frame_stub_ = app_framework;

    // protobuf 版本校验
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // 读取配置文件
    int ret = SetUpAndCheckConfig();
    if (ret != 0)
    {
        LOG(ERROR)<<"set up config failed";
        return -1;
    }

    // 创建后端连接
    for (int i=0; i<config_.backends_size(); ++i)
    {
        const connector_config::EndPoint& end_point = config_.backends(i);
        EasyNet* net = frame_stub_->CreateBackEnd(end_point.uri().c_str(), 0);
        if (NULL == net)
        {
            LOG(ERROR)<<"create backend failed, "<<"uri: "<<end_point.uri();
            return -1;
        }

        BackEndSet::AddBackEnd(net);
    }

    return 0;
}

int ConnectorApp::OnFini()
{
    ::google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

int ConnectorApp::OnStop()
{
    return 0;
}
int ConnectorApp::OnResume()
{
    return 0;
}

int ConnectorApp::OnUpdate()
{
    return 0;
}
int ConnectorApp::OnReload()
{
    return 0;
}

int ConnectorApp::OnRecvCtrl(int argc, char** argv)
{
    return 0;
}

// 新连接建立，发起与后端的握手
int ConnectorApp::OnFrontEndConnect(EasyNet* net, int type)
{
    if (NULL == frame_stub_ || NULL == net)
    {
        LOG(ERROR)<<"assert failed, null frame stub | null conn session mgr";
        return -1;
    }

    LOG(INFO)<<"frontend:"<<net->uri()<<" connect successfully";

    void* uctx = frame_stub_->GetUserContext(net);
    if (NULL == uctx)
    {
        LOG(ERROR)<<"null user context";
        return -1;
    }

    // context 初始化
    ConnectorSession* session = new(uctx) ConnectorSession();
    if (NULL == session)
    {
        LOG(ERROR)<<"assert failed";
        return -1;
    }

    int ret = session->Init(net, frame_stub_);
    if (ret != 0)
    {
        LOG(ERROR)<<"init user session failed";
        return -1;
    }

    return 0;
}

// frontend关闭连接
int ConnectorApp::OnFrontEndClose(EasyNet* net, int type)
{
    if (NULL == frame_stub_ || NULL == net)
    {
        LOG(ERROR)<<"assert failed, null frame stub | null conn session mgr";
        return -1;
    }

    LOG(INFO)<<"frontend:"<<net->uri()<<" close actively or closed for socket error";

    void* uctx = frame_stub_->GetUserContext(net);
    if (NULL == uctx)
    {
        LOG(ERROR)<<"null user context";
        return -1;
    }

    // context资源释放
    // session进行CleanUp后，会随着net的回收而回收
    ConnectorSession* session = static_cast<ConnectorSession*>(uctx);
    session->StopBackEndSession();
    session->CleanUp();

    return 0;
}

int ConnectorApp::OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    if (NULL == frame_stub_)
    {
        LOG(ERROR)<<"assert failed, null frame stub | null conn session mgr";
        return -1;
    }

    // context 查找
    void* uctx = frame_stub_->GetUserContext(net);
    if (NULL == uctx)
    {
        LOG(ERROR)<<"null user context";
        return -1;
    }
    ConnectorSession* session = static_cast<ConnectorSession*>(uctx);
    assert(session != NULL);

    static connector_client::CSMsg up_msg;
    up_msg.ParseFromArray(msg, size);

    // 检查上行包消息是否合法
    if (session->CheckUpSideMessage(up_msg) != 0)
    {
        return -1;
    }

    if (!session->BeenReady())
    {
        LOG(INFO)<<"handshake msg from client"<<std::endl<<up_msg.DebugString();

        if (session->DriveStateMachine() < 0)
        {
            // 删除net时，会同时清理session空间
            LOG(ERROR)<<"state machine run failed, close frontend";
            frame_stub_->DestroyFrontEnd(&net);
            return -1;
        }

        // 目前无排队，鉴权通过直接发起START
        if (STATE_SYNING == session->state())
        {
            if (session->DriveStateMachine() < 0)
            {
                // 删除net时，会同时清理session空间
                LOG(ERROR)<<"state machine run failed, close frontend";
                frame_stub_->DestroyFrontEnd(&net);
                return -1;
            }
        }
    }
    else
    {
        LOG(INFO)<<"upside msg from client: "<<std::endl<<up_msg.DebugString();

        if (up_msg.head().bodyid() != connector_client::DATA_TRANSPARENT)
        {
            LOG(ERROR)<<"cmd id:"<<up_msg.head().bodyid()<<
                ", NOT cmd DATA_TRANSPARENT, unexpected";
            return -1;
        }

        return session->ForwardUpSideMessage(up_msg.head().sequence(),
                                             up_msg.body().data());
    }
}

int ConnectorApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    assert(frame_stub_ != NULL);

    // 根据后端回调，转发
    connector_server::SSMsg down_msg;
    down_msg.ParseFromArray(msg, size);

    LOG(INFO)<<"downside resp from server: "<<std::endl<<down_msg.DebugString();
    EasyNet* front_net = frame_stub_->GetFrontEndByAsyncIds(
                                        down_msg.head().session().fd(),
                                        down_msg.head().session().nid());
    if (NULL == front_net)
    {
        LOG(ERROR)<<"no net found by session fd:"<<
            down_msg.head().session().fd()<<", nid:"<<
            down_msg.head().session().nid();
        return -1;
    }

    ConnectorSession* session = static_cast<ConnectorSession*>(
                                    frame_stub_->GetUserContext(front_net));
    if (NULL == session)
    {
        LOG(ERROR)<<"no session found by frontend net";
        return -1;
    }

    // 中转至前端网络连接，同时update状态机
    if (connector_server::DATA != down_msg.head().bodyid())
    {
        //assert (session->state() != STATE_OK && STATE_SYNACK == session->state());
        if (connector_server::FIN != down_msg.head().bodyid() && STATE_OK == session->state())
        {
            LOG(ERROR)<<"state ok, but recv pkg NOT FIN and DATA, ignore";
            return 0;
        }

        if (session->DriveStateMachine() < 0)
        {
            // 删除net时，会同时清理session空间
            LOG(ERROR)<<"state machine run failed, close frontend";
            frame_stub_->DestroyFrontEnd(&front_net);
            return -1;
        }
        else
        {
            session->set_sid(down_msg.head().session().sid());
            return 0;
        }
    }
    else
    {
// 由于目前后端测试工具不发送ack握手包，为了联调方便，debug版本不检查
#ifndef _DEBUG
        if (session->state() != STATE_OK)
        {
            LOG(ERROR)<<"state NOT been STATE_OK, data can NOT transfer";
            frame_stub_->DestroyFrontEnd(&front_net);
            return -1;
        }

        if (session->sid() != down_msg.head().session().sid())
        {
            LOG(ERROR)<<"session id NOT unexpected. local sid:"<<session->sid()
                <<", down sid:"<<down_msg.head().session().sid();
            frame_stub_->DestroyFrontEnd(&front_net);
            return -1;
        }
#endif

        return session->ForwardDownSideMessage(down_msg.head().sequence(),
                                               down_msg.body().data().data().c_str(),
                                               down_msg.body().data().data().size());
    }
}

int ConnectorApp::OnTimer(EasyTimer* timer, int timer_id)
{
    // TODO 后端心跳管理
    return 0;
}

int ConnectorApp::OnReportRundata()
{
    return 0;
}

// 这里必须大小和实际ConnectorSession完全一致
size_t ConnectorApp::GetFrontEndContextSize()
{
    return sizeof(ConnectorSession);
}

size_t ConnectorApp::GetBackEndContextSize()
{
    return 0;
}

size_t ConnectorApp::GetFrontEndMaxMsgSize()
{
    return 0;
}

size_t ConnectorApp::GetBackEndMaxMsgSize()
{
    return 0;
}

const char* ConnectorApp::GetAppVersion()
{
    return "1.0.0";
}
