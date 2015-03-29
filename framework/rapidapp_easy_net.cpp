#include "rapidapp_easy_net.h"
#include "utils/tcp_socket.h"
#include <glog/logging.h>
#include <cassert>
#include <cstdlib>

namespace rapidapp {

static int nid_seed = 1;
EasyNet::EasyNet() : hevent_(NULL), net_type_(0), nid_(nid_seed++), state_(NET_INIT),
                      last_active_timestamp_(0), prev_pkg_count_(0), curr_pkg_count_(0),
                      prev_traffic_bytes_(0), curr_traffic_bytes_(0)
{
    uri_[0] = '\0';
    rpc_binded_ = NULL;
    user_context_ = NULL;
}

EasyNet::~EasyNet()
{}

int EasyNet::Init(evutil_socket_t sock_fd, int type, struct event_base* ev_base)
{
    if (sock_fd < 0 || NULL == ev_base)
    {
        PLOG(ERROR)<<"sock_fd:("<<sock_fd<<") < 0 || null event_base";
        return -1;
    }

    evutil_make_socket_nonblocking(sock_fd);
    evutil_make_socket_closeonexec(sock_fd);
    tcpsocket_set_nodelay(sock_fd);

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
    net_type_ = type;
    state_ = NET_ESTABLISHED;

    return 0;
}

int EasyNet::Connect(const char* uri, int type, struct event_base* ev_base)
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
    int ret = tcpsocket_str2sockin(uri, &sin);
    if (ret != 0)
    {
        PLOG(ERROR)<<"get sockaddr_in failed by uri:"<<uri;
        return -1;
    }

    struct bufferevent* bev = bufferevent_socket_new(ev_base, -1/*invalid fd*/,
                                                     BEV_OPT_CLOSE_ON_FREE);
    if (NULL == bev)
    {
        PLOG(ERROR)<<"create bufferevent for uri:"<<uri<<" failed";
        return -1;
    }
    tcpsocket_set_nodelay(bufferevent_getfd(bev));
    bufferevent_enable(bev, EV_READ|EV_WRITE);

    // socket fd没有设置的情况下，调用connect会创建一个socket并且设置为非阻塞模式
    ret = bufferevent_socket_connect(bev, (struct sockaddr*)&sin,
                                     sizeof(sin));
    if (ret != 0)
    {
        PLOG(ERROR)<<"start connect to uri:"<<uri<<" failed";
        bufferevent_free(bev);
        return -1;
    }

    hevent_ = bev;
    net_type_ = type;
    state_ = NET_CONNECTING;
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

int EasyNet::CreateUserContext(size_t uctx_size)
{
    if (0 == uctx_size)
    {
        return 0;
    }

    user_context_ = calloc(1, uctx_size);
    if (NULL == user_context_)
    {
        PLOG(ERROR)<<"calloc context size:"<<uctx_size<<" failed";
        return -1;
    }

    return 0;
}

void EasyNet::DestroyUserContext()
{
    if (NULL != user_context_)
    {
        free(user_context_);
        user_context_ = NULL;
    }
}

int EasyNet::Send(const char* msg, size_t size)
{
    if (NULL == msg || 0 == size)
    {
        return EASY_NET_OK;
    }

    assert(hevent_ != NULL);

    if (state_ != NET_ESTABLISHED)
    {
        if (state_ != NET_FAILED)
        {
            return EASY_NET_ERR_NOT_YET_ESTABLISHED;
        }
        else
        {
            return EASY_NET_ERR_ESTABLISH_FAILED;
        }
    }

    // 由于目前bufferevent不支持writev，这里调用2次
    uint32_t msglen = htonl(size + sizeof(msglen));
    if (0 != bufferevent_write(hevent_, &msglen, sizeof(msglen)))
    {
        PLOG(ERROR)<<"write msglen to uri:"<<uri_<<"sock fd:"<<
            bufferevent_getfd(hevent_)<<" failed";
        return EASY_NET_ERR_SEND_ERROR;
    }

    if (0 != bufferevent_write(hevent_, msg, size))
    {
        PLOG(ERROR)<<"write to uri:"<<uri_<<"sock fd:"<<
            bufferevent_getfd(hevent_)<<" failed";
        return EASY_NET_ERR_SEND_ERROR;
    }

    LOG(INFO)<<"write to uri:"<<uri_<<"sock fd:"<<bufferevent_getfd(hevent_)<<" successfully";

    return EASY_NET_OK;
}

}
