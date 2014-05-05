#include "rapidapp_net.h"

namespace rapidapp {
EasyNet::EasyNet()
{}

EasyNet::~EasyNet()
{}

int EasyNet::SendToFrontEnd(const char* buf, size_t buf_size)
{
    if (NULL == buf || 0 == buf_size)
    {
        return -1;
    }

    return 0;
}

int EasyNet::SendToBackEnd(const char* buf, size_t buf_size)
{
    if (NULL == buf || 0 == buf_size)
    {
        return -1;
    }

    return 0;
}

EasyNet* EasyNet::CreateFrontEnd(const char* url)
{
    if (NULL == url)
    {
        return NULL;
    }

    return NULL;
}

EasyNet* EasyNet::CreateBackEnd(const char* url)
{
    if (NULL == url)
    {
        return NULL;
    }

    return NULL;
}

void EasyNet::DestroyFrontEnd(EasyNet** net)
{
    if (NULL == net || NULL == *net)
    {
        return;
    }
}

void EasyNet::DestroyBackEnd(EasyNet** net)
{
    if (NULL == net || NULL == *net)
    {
        return;
    }
}

}
