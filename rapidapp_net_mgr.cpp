#include "rapidapp_net_mgr.h"
#include <cstdlib>

namespace rapidapp {

const int kDefaultUpSize = 1024 * 1024 * 1024;  // 1M

NetHandlerMgr::NetHandlerMgr()
{
    recv_buffer_.buffer = NULL;
    recv_buffer_.size = 0;
}

NetHandlerMgr::~NetHandlerMgr()
{}

int NetHandlerMgr::Init(size_t recv_buff_size)
{
    if (0 == recv_buff_size)
    {
        recv_buff_size = kDefaultUpSize;
    }

    recv_buffer_.buffer = static_cast<char*>(calloc(recv_buff_size, 1));
    if (NULL == recv_buffer_.buffer)
    {
        return -1;
    }
    recv_buffer_.size = recv_buff_size;

    return 0;
}

void NetHandlerMgr::CleanUp()
{
    if (recv_buffer_.buffer != NULL)
    {
        free(recv_buffer_.buffer);
        recv_buffer_.buffer = NULL;
    }
}

}
