#include "rapidapp_framework_imp.h"

namespace rapidapp {
AppFrameWork::AppFrameWork()
{}

AppFrameWork::~AppFrameWork()
{}

int AppFrameWork::Init(size_t recv_buff_size)
{
    net_mgr_ = new NetHandlerMgr();
    if (NULL == net_mgr_)
    {
        return -1;
    }

    return net_mgr_->Init(recv_buff_size);
}

void AppFrameWork::CleanUp()
{
    if (net_mgr_ != NULL)
    {
        net_mgr_->CleanUp();

        delete net_mgr_;
        net_mgr_ = NULL;
    }
}

int AppFrameWork::SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size)
{
    if (NULL == buf || 0 == buf_size)
    {
        return -1;
    }

    return 0;
}

int AppFrameWork::SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size)
{
    if (NULL == buf || 0 == buf_size)
    {
        return -1;
    }

    return 0;
}

EasyNet* AppFrameWork::CreateBackEnd(const char* url)
{
    if (NULL == url)
    {
        return NULL;
    }

    return NULL;
}

void AppFrameWork::DestroyBackEnd(EasyNet** net)
{
    if (NULL == net || NULL == *net)
    {
        return;
    }
}

}
