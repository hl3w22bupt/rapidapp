/*
 * connector接入服务器。
 * connector主要功能：
 * 1. 连接管理
 * 2. 广播
 * 3. 串行汇聚
 * 4. 过载保护
 * 5. 断线重连
 * 6. traffic control
 * 7. 无状态，方便平行扩展
 *
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

ConnectorApp::ConnectorApp() : frame_stub_(NULL), backend_pos_(-1)
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

    backend_pos_ = 0;
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

int ConnectorApp::OnRecvCtrl(const char* msg)
{
    return 0;
}

int ConnectorApp::OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    if (NULL == frame_stub_)
    {
        LOG(ERROR)<<"assert failed, null frame stub | null conn session mgr";
        return -1;
    }

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

    int ret = session->Init(net);
    if (ret != 0)
    {
        LOG(ERROR)<<"init user session failed";
        return -1;
    }

    connector_client::CSMsg up_msg;
    up_msg.ParseFromArray(msg, size);
    LOG(INFO)<<"upside req: "<<std::endl<<up_msg.DebugString();

    // TODO 根据路由规则，转发

    ++backend_pos_;
    if (backend_pos_ >= config_.backends_size())
    {
        backend_pos_ = 0;
    }

    return 0;
}

int ConnectorApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    assert(frame_stub_ != NULL);

    // 根据后端回调，转发
    connector_server::SSMsg down_msg;
    down_msg.ParseFromArray(msg, size);

    LOG(INFO)<<"downside resp: "<<std::endl<<down_msg.DebugString();
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
        session->ChangeState(0);
    }
    else
    {
        if (session->state() != STATE_OK)
        {
            LOG(ERROR)<<"state NOT been STATE_OK, data can NOT transfer";
            return -1;
        }

        connector_client::CSMsg down_msg_2client;
        down_msg_2client.mutable_head()->set_magic(0x3344);
        down_msg_2client.mutable_head()->set_sequence(1);
        down_msg_2client.mutable_head()->set_bodyid(connector_client::DATA_TRANSPARENT);
        down_msg_2client.mutable_body()->set_data(down_msg.body().data().data());
        std::string down_buff;
        down_msg_2client.SerializeToString(&down_buff);

        int ret = frame_stub_->SendToFrontEnd(front_net,
                                              down_buff.c_str(), down_buff.size());
        if (ret != 0)
        {
            LOG(ERROR)<<"transfer to frontend failed";
            return -1;
        }
    }

    return 0;
}

int ConnectorApp::OnTimer(EasyTimer* timer, int timer_id)
{
    // TODO 空闲连接检查
    return 0;
}

int ConnectorApp::OnReportRundata()
{
    return 0;
}

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
