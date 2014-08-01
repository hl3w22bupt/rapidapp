#include "../client.pb.h"
#include "utils/rap_net_uri.h"
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <gflags/gflags.h>
#include <iostream>
#include <errno.h>

bool connected = false;
bool ready_to_exit = false;

typedef struct {
    struct bufferevent* bev;
    struct event_base* evbase;
} CTX;

DEFINE_int32(timer_us, 1000, "timer time period");
DEFINE_string(uri, "tcp:127.0.0.1:8888", "connector server uri");

void on_timer_cb_func(evutil_socket_t fd, short what, void *arg) {
    std::cout<<"timer trigger"<<std::endl;
    if (NULL == arg)
        return;

    CTX* ctx = static_cast<CTX*>(arg);

    if (ready_to_exit)
    {
        event_base_loopbreak(ctx->evbase);
        return;
    }

    if (connected)
    {
        std::cout<<"send request to connector"<<std::endl;
        static size_t seq_seed = 0;
        connector_client::CSMsg msg;
        msg.mutable_head()->set_magic(0x3344);
        msg.mutable_head()->set_sequence(++seq_seed);
        msg.mutable_head()->set_bodyid(connector_client::DATA_TRANSPARENT);
        msg.mutable_body()->set_data("abcdefghijklmn");

        std::string data_to_send;
        msg.SerializeToString(&data_to_send);

        uint32_t msglen = htonl(msg.ByteSize() + sizeof(uint32_t));
        bufferevent_write(ctx->bev, &msglen, sizeof(msglen));
        bufferevent_write(ctx->bev, data_to_send.c_str(), data_to_send.size());
    }
}

void on_readable_cb_func(struct bufferevent* bev, void *arg) {
}

void on_eventable_cb_func(struct bufferevent* bev, short events, void *arg) {
    if (events & BEV_EVENT_ERROR) {
        std::cout<<"connect failed, for errno:"<<errno<<std::endl;
        ready_to_exit = true;
    }

    if (events & BEV_EVENT_CONNECTED) {
        std::cout<<"connect success"<<std::endl;
        connected = true;
    }
}

int main(int argc, char** argv)
{
    gflags::SetUsageMessage("connector client");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    struct event_base* ev_base = event_base_new();
    if (NULL == ev_base)
    {
        std::cout<<"event_base_new failed"<<std::endl;
        return -1;
    }

    struct sockaddr_in sin;
    int ret = rap_uri_get_socket_addr(FLAGS_uri.c_str(), &sin);
    if (ret != 0)
    {
        std::cout<<"get sockaddr_in failed by uri:"<<FLAGS_uri<<std::endl;
        return -1;
    }

    struct bufferevent* bev = bufferevent_socket_new(ev_base, -1/*invalid fd*/,
                                                     BEV_OPT_CLOSE_ON_FREE);
    if (NULL == bev)
    {
        std::cout<<"create bufferevent for uri:"<<FLAGS_uri<<" failed"<<std::endl;
        return -1;
    }
    bufferevent_enable(bev, EV_READ|EV_WRITE);

    // socket fd没有设置的情况下，调用connect会创建一个socket并且设置为非阻塞模式
    ret = bufferevent_socket_connect(bev, (struct sockaddr*)&sin,
                                     sizeof(sin));
    if (ret != 0)
    {
        std::cout<<"start connect to uri:"<<FLAGS_uri<<" failed"<<std::endl;
        bufferevent_free(bev);
        return -1;
    }

    bufferevent_setcb(bev, on_readable_cb_func, NULL,
                      on_eventable_cb_func, 0);

    CTX ctx;
    ctx.bev = bev;
    ctx.evbase = ev_base;
    struct event* timer = event_new(ev_base, -1, EV_PERSIST,
                                    on_timer_cb_func, &ctx);
    if (NULL == timer)
    {
        std::cout<<"event new timer failed"<<std::endl;
        return -1;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = FLAGS_timer_us;
    if (event_add(timer, &tv) != 0)
    {
        std::cout<<"event add timer failed"<<std::endl;
        return -1;
    }

    event_base_dispatch(ev_base);
    gflags::ShutDownCommandLineFlags();

    return 0;
}
