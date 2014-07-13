/*
 * connector接入服务器。
 * connector主要功能：
 * 1. 连接管理
 * 2. 广播
 * 3. 串行汇聚
 * 4. 过载保护
 * 5. 断线重连
 * 6. 无状态，方便平行扩展
 *
 * */
#include "connector_app.h"
#include <gflags/gflags.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <sstream>
#include <fstream>
#include <iostream>

DEFINE_string(config_file, "config.ini", "config file path name");

ConnectorApp::ConnectorApp() : frame_stub_(NULL), conn_session_mgr_(NULL)
{}

ConnectorApp::~ConnectorApp()
{}

int ConnectorApp::OnInit(IFrameWork* app_framework)
{
    if (NULL == app_framework)
    {
        LOG(ERROR)<<"null app framework, assert failed";
        return -1;
    }

    frame_stub_ = app_framework;

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    FILE* fp = fopen(FLAGS_config_file.c_str(), "r");
    if (NULL == fp)
    {
        PLOG(ERROR)<<"open file: "<<FLAGS_config_file<<" failed";
        return -1;
    }

    rewind(fp);
    long filesize = ftell(fp);
    char* conf = static_cast<char*>(calloc(1, filesize));
    if (NULL == conf)
    {
        LOG(ERROR)<<"calloc size: "<<filesize<<" failed";
        fclose(fp);
        return -1;
    }

    if (NULL == fgets(conf, filesize, fp))
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

    LOG(INFO)<<"initialize from conf file: "<<FLAGS_config_file;

    // 连接的上下文池，状态机驱动
    conn_session_mgr_ = new(std::nothrow) ConnectorSessionMgr(config_.max_online_num());
    if (NULL == conn_session_mgr_)
    {
        PLOG(ERROR)<<"new ConnectorSessionMgr failed";
        return -1;
    }

    int ret = conn_session_mgr_->Init();
    if (ret != 0)
    {
        LOG(ERROR)<<"connector session mgr initialized failed";
        return -1;
    }

    // TODO 创建后端连接
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
    if (NULL == frame_stub_ || NULL == conn_session_mgr_)
    {
        LOG(ERROR)<<"assert failed, null frame stub | null conn session mgr";
        return -1;
    }

    ConnectorSession* session = conn_session_mgr_->CreateInstance(net);
    if (NULL == session)
    {
        LOG(ERROR)<<"create session instance failed";
        return -1;
    }

    // TODO 根据路由规则，转发

    return 0;
}

int ConnectorApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    // TODO 根据后端回调，转发
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

size_t ConnectorApp::GetFrontEndMaxMsgSize()
{
    return 0;
}

size_t ConnectorApp::GetBackEndMaxMsgSize()
{
    return 0;
}

size_t ConnectorApp::GetFrontEndMsgLength(const char* buffer, size_t size)
{
    if (NULL == buffer || 4 > size)
    {
        return 0;
    }

    uint32_t len = 0;
    memcpy(&len, buffer, sizeof(uint32_t));
    return ntohl(len);
}

size_t ConnectorApp::GetBackEndMsgLength(int type, const char* buffer, size_t size)
{
    if (NULL == buffer || 4 > size)
    {
        return 0;
    }

    uint32_t len = 0;
    memcpy(&len, buffer, sizeof(uint32_t));
    return ntohl(len);
}

const char* ConnectorApp::GetAppVersion()
{
    return "1.0.0";
}
