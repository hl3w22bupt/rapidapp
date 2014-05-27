#include "rapidapp_framework_imp.h"

namespace rapidapp {
AppFrameWork::AppFrameWork() : net_mgr_()
{}

AppFrameWork::~AppFrameWork()
{}

int AppFrameWork::Init(size_t recv_buff_size)
{
    return net_mgr_.Init(recv_buff_size);
}

void AppFrameWork::CleanUp()
{
    net_mgr_.CleanUp();
}

int AppFrameWork::SendToFrontEnd(EasyNet* net, const char* buf, size_t buf_size)
{
    if (NULL == net || NULL == buf || 0 == buf_size)
    {
        return -1;
    }

    return 0;
}

int AppFrameWork::SendToBackEnd(EasyNet* net, const char* buf, size_t buf_size)
{
    if (NULL == net || NULL == buf || 0 == buf_size)
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
