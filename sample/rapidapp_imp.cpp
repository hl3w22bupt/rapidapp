#include "rapidapp_imp.h"
#include "sample.pb.h"
#include <google/protobuf/message.h>

MyApp::MyApp()
{}

MyApp::~MyApp()
{}

int MyApp::OnInit(IFrameWork* app_framework)
{
    if (NULL == app_framework)
    {
        LOG(ERROR)<<"null app framework";
        return -1;
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    frame_stub_ = app_framework;
    return 0;
}

int MyApp::OnFini()
{
    ::google::protobuf::ShutdownProtobufLibrary();

    return 0;
}

int MyApp::OnStop()
{
    return 0;
}
int MyApp::OnResume()
{
    return 0;
}

int MyApp::OnUpdate()
{
    return 0;
}
int MyApp::OnReload()
{
    return 0;
}

int MyApp::OnRecvCtrl(const char* msg)
{
    return 0;
}

int MyApp::OnRecvFrontEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    static uint64_t msg_seq = 0;
    LOG(INFO)<<"recv app msg size:"<<size;

    // TODO parse msg from frontend
    // COdeInputStream avoid copying bytes to a seperate buffer

    rapidapp_sample::Mesg resp;
    resp.mutable_body()->set_seq(++msg_seq);
    resp.mutable_body()->set_echo("rapidapp sample");
    // 先保证调用set msglen，同时保证msglen定义为fixed固定大小，以计算出正确的序列化后大小
    resp.set_msglen(0);
    resp.set_msglen(resp.ByteSize());

    LOG(INFO)<<"resp to frontend:"<<std::endl<<resp.DebugString();

    std::string resp_str;
    resp.SerializeToString(&resp_str);

    //rapidapp_sample::Mesg resp_unpack;
    //resp_unpack.ParseFromString(resp_str);

    if (NULL == frame_stub_)
    {
        LOG(ERROR)<<"assert failed, null frame stub";
        return -1;
    }
    frame_stub_->SendToFrontEnd(net, resp_str.c_str(), resp.ByteSize());

    return 0;
}

int MyApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    return 0;
}

int MyApp::OnTimer(EasyTimer* timer, int timer_id)
{
    return 0;
}

int MyApp::OnReportRundata()
{
    return 0;
}

size_t MyApp::GetFrontEndMaxMsgSize()
{
    return 0;
}

size_t MyApp::GetBackEndMaxMsgSize()
{
    return 0;
}

size_t MyApp::GetFrontEndMsgLength(const char* buffer, size_t size)
{
    if (NULL == buffer || 4 > size)
    {
        return 0;
    }

#ifndef _DEBUG
    uint32_t len = 0;
    memcpy(&len, buffer, sizeof(uint32_t));
    return ntohl(len);
#else
    return size;
#endif
}

size_t MyApp::GetBackEndMsgLength(int type, const char* buffer, size_t size)
{
    if (NULL == buffer || 4 > size)
    {
        return 0;
    }

    uint32_t len = 0;
    memcpy(&len, buffer, sizeof(uint32_t));
    return ntohl(len);
}

const char* MyApp::GetAppVersion()
{
    return "1.0.0";
}
