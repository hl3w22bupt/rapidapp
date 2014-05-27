#include "rapidapp_net_mgr.h"
#include <cstdlib>

namespace rapidapp {

const int kDefaultUpSize = 1024 * 1024 * 1024;  // 1M

ConnectionHandlerMgr::ConnectionHandlerMgr()
{
    up_msg_buffer_.buffer = NULL;
    up_msg_buffer_.size = 0;
}

ConnectionHandlerMgr::~ConnectionHandlerMgr()
{}

int ConnectionHandlerMgr::Init(size_t up_size)
{
    if (0 == up_size)
    {
        up_size = kDefaultUpSize;
    }

    up_msg_buffer_.buffer = static_cast<char*>(calloc(up_size, 1));
    if (NULL == up_msg_buffer_.buffer)
    {
        return -1;
    }
    up_msg_buffer_.size = up_size;

    return 0;
}

void ConnectionHandlerMgr::CleanUp()
{
    if (up_msg_buffer_.buffer != NULL)
    {
        free(up_msg_buffer_.buffer);
        up_msg_buffer_.buffer = NULL;
    }
}

}
