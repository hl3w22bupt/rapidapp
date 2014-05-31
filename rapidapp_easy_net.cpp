#include "rapidapp_easy_net.h"
#include "utils/rap_net_uri.h"
#include <glog/logging.h>
#include <cassert>

namespace rapidapp {

EasyNet::EasyNet() : hevent_(NULL)
{
    uri_[0] = '\0';
}

EasyNet::~EasyNet()
{}

int EasyNet::Init(evutil_socket_t sock_fd, struct event_base* ev_base)
{
    if (sock_fd < 0 || NULL == ev_base)
    {
        PLOG(ERROR)<<"sock_fd:("<<sock_fd<<") < 0 || null event_base";
        return -1;
    }

    evutil_make_socket_nonblocking(sock_fd);
    evutil_make_socket_closeonexec(sock_fd);
    struct bufferevent* bev =
        bufferevent_socket_new(ev_base, sock_fd,
                               BEV_OPT_CLOSE_ON_FREE);
    if (NULL == bev)
    {
        PLOG(ERROR)<<"add new connection fd:"<<sock_fd<<" failed";
        return -1;
    }
    bufferevent_enable(bev, EV_READ|EV_WRITE);

    hevent_ = bev;

    return 0;
}

int EasyNet::Connect(const char* uri, struct event_base* ev_base)
{
    if (NULL == uri || NULL == ev_base)
    {
        PLOG(ERROR)<<"null uri || null event_base";
        return -1;
    }

    if (strlen(uri) >= MAX_URL_LEN)
    {
        PLOG(ERROR)<<"length of uri:"<<uri<<" >= MAX_URL_LEN";
        return -1;
    }

    struct sockaddr_in sin;
    int ret = rap_uri_get_socket_addr(uri, &sin);
    if (ret != 0)
    {
        PLOG(ERROR)<<"get sockaddr_in failed by uri:"<<uri;
        return -1;
    }

    struct bufferevent* bev = bufferevent_socket_new(ev_base, -1,
                                                     BEV_OPT_CLOSE_ON_FREE);
    if (NULL == bev)
    {
        PLOG(ERROR)<<"create bufferevent for uri:"<<uri<<" failed";
        return -1;
    }
    bufferevent_enable(bev, EV_READ|EV_WRITE);

    // socket fd没有设置的情况下，调用connect会创建一个socket并且设置为非阻塞模式
    ret = bufferevent_socket_connect(hevent_, (struct sockaddr*)&sin,
                                     sizeof(sin));
    if (ret != 0)
    {
        PLOG(ERROR)<<"start connect to uri:"<<uri<<" failed";
        bufferevent_free(bev);
        return -1;
    }

    hevent_ = bev;
    strcpy(uri_, uri);

    return 0;
}

void EasyNet::CleanUp()
{
    if (hevent_ != NULL)
    {
        bufferevent_free(hevent_);
        hevent_ = NULL;
    }
}

int EasyNet::Send(const char* msg, size_t size)
{
    if (NULL == msg || 0 == size)
    {
        return 0;
    }

    assert(hevent_ != NULL);

    if (0 != bufferevent_write(hevent_, msg, size))
    {
        PLOG(ERROR)<<"read from uri:"<<uri_<<"sock fd:"<<
            bufferevent_getfd(hevent_)<<"failed";
        return -1;
    }

    return 0;
}

}
