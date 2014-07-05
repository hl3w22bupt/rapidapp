#include "rapidapp_imp.h"
#include "sample.pb.h"
#include <google/protobuf/message.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/io/coded_stream.h>

class RpcHandlerImp : public IMsgHandler {
    public:
        RpcHandlerImp(){}
        ~RpcHandlerImp(){}

    public:
        int HandleRequest(const void* request, void* data, size_t* size) {
            if (NULL == request || NULL == data || NULL == size || 0 == *size)
                return -1;

            const rapidapp_sample::Mesg* req = static_cast<const rapidapp_sample::Mesg*>(request);
            req->SerializeToArray(data, *size);
            *size = req->ByteSize();

            return 0;
        }

        int HandleResponse(const void* data, size_t size, void* response) {
            if (NULL == data || 0 == size || NULL == response)
                return -1;

            rapidapp_sample::Mesg* resp = static_cast<rapidapp_sample::Mesg*>(response);
            resp->ParseFromArray(data, size);

            return 0;
        }
};

RpcHandlerImp rpc_handler;

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

    const char* uri = "tcp:127.0.0.1:8081";
    EasyNet* net = frame_stub_->CreateBackEnd(uri, 0);
    if (NULL == net)
    {
        LOG(ERROR)<<"CreateBackEnd "<<uri<<" failed";
        return -1;
    }
    backend_ = net;

    rpc_ = new(std::nothrow) EasyRpc();
    if (NULL == rpc_)
    {
        return -1;
    }

    int ret = rpc_->Init(net, &rpc_handler, 1000, 1000);
    if (ret != 0)
    {
        LOG(ERROR)<<"rpc init failed";
        return -1;
    }

    return 0;
}

int MyApp::OnFini()
{
    if (rpc_ != NULL)
    {
        delete rpc_;
        rpc_ = NULL;
    }

    if (backend_ != NULL)
    {
        frame_stub_->DestroyBackEnd(&backend_);
        backend_ = NULL;
    }
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

    rapidapp_sample::Mesg backend_resp;
    LOG(INFO)<<"start rpc call...";
    rpc_->RpcCall(&resp, &backend_resp);
    //frame_stub_->SendToBackEnd(backend_, resp_str.c_str(), resp.ByteSize());
    return 0;
}

int MyApp::OnRecvBackEnd(EasyNet* net, int type, const char* msg, size_t size)
{
    rapidapp_sample::Mesg resp;
    resp.ParseFromArray(msg, size);

    LOG(INFO)<<"resp from backend:"<<std::endl<<resp.DebugString();
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

#ifndef _DEBUG
    uint32_t len = 0;
    memcpy(&len, buffer, sizeof(uint32_t));
    return ntohl(len);
#else
    return size;
#endif
}

const char* MyApp::GetAppVersion()
{
    return "1.0.0";
}
