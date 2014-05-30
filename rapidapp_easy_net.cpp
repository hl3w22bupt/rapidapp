#include "rapidapp_easy_net.h"
#include "utils/rap_net_uri.h"
#include <glog/logging.h>

namespace rapidapp {

EasyNet::EasyNet() : hevent_(NULL)
{}

EasyNet::~EasyNet()
{}

int EasyNet::Init(const char* uri, IEventListener* event_listener,
                  struct event_base* ev_base)
{
    if (NULL == uri || NULL == event_listener || NULL == ev_base)
    {
        return -1;
    }

    evutil_socket_t sock_fd = rap_uri_open_socket(uri);
    if (sock_fd < 0)
    {
        PLOG(ERROR)<<"open socket failed by uri:"<<uri;
        return -1;
    }

    evutil_make_socket_nonblocking(sock_fd);
    evutil_make_socket_closeonexec(sock_fd);
    hevent_ = bufferevent_socket_new(ev_base, sock_fd,
                                     BEV_OPT_CLOSE_ON_FREE);
    if (NULL == hevent_)
    {
        PLOG(ERROR)<<"create bufferevent for uri:"<<uri<<" failed";
        return -1;
    }
    bufferevent_enable(hevent_, EV_READ|EV_WRITE);

    return 0;
}

void EasyNet::CleanUp()
{}

}
