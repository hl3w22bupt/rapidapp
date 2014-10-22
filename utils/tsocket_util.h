#ifndef TSOCKET_UTIL_H_
#define TSOCKET_UTIL_H_

#include <string>
#include "tcp_socket.h"

namespace tsocket_util {

enum NET_STATE {
    NET_INITED      = 0,
    NET_CONNECTING  = 1,
    NET_CONNECTED   = 2,
    NET_SEND_FAILED = 3,
    NET_RECV_FAILED = 4,
    NET_PEER_CLOSED = 5,
    NET_ASSERT_FAIL = 6,
};

enum {
    NORMAL           = 0,
    INVLD_ARGUMENT   = -1,
    INVLD_URL        = -2,
    MALLOC_FAILED    = -3,
    SEND_BUFF_FULL   = -50,
    SEND_BUFF_BUSY   = -51,
    RECV_BUFF_FULL   = -52,
    RECV_BUFF_BUSY   = -53,
    CONNECT_FAILED   = -101,
    NOT_CONNECTED    = -102,
    SEND_FAILED      = -201,
    SEND_UNCOMPLETED = -202,
    RECV_FAILED      = -301,
    PEER_CLOSED      = -302,
    ASSERT_FAILED    = -1000,
};

// ring buffer
struct RingBuffer {
    size_t buff_size;
    char* buff;
    size_t begin;
    size_t end;

#ifdef TSOCKET_UTIL_ATOMIC
    volatile int atomic_in;
    volatile int atomic_out;
#endif

    RingBuffer() {
        buff = NULL;
        buff_size = 0;

        begin = 0;
        end = 0;

#ifdef TSOCKET_UTIL_ATOMIC
        atomic_in = 0;
        atomic_out = 0;
#endif
    };  //lint !e1401

    ~RingBuffer() {}; //lint !e1540

    bool Malloc(size_t size) {
        buff = static_cast<char*>(calloc(1, size));
        return (NULL == buff) ? false : true;
    };

    void Free() {
        if (buff_size > 0 && buff != NULL) {
            free(buff);
            buff_size = 0;
        }
    };

    inline char* Ptr() {return buff;};
    inline size_t Size() const {return buff_size;};
    inline int Avaiable() const {return buff_size - end;};
    inline int Datalen() const {return end - begin;};
#ifdef TSOCKET_UTIL_ATOMIC
    inline bool Locked() const {return (atomic_in != atomic_out) ? true : false;}
#endif

    inline void Recycle_ATOMIC() {
#ifdef TSOCKET_UTIL_ATOMIC
        if (Locked())
            return;
#endif

        if(begin > 0) {
#ifdef TSOCKET_UTIL_ATOMIC
            ++atomic_in;
#endif

            if(end > begin)
                memmove(buff, buff + begin, end - begin);
            end -= begin;
            begin = 0;

#ifdef TSOCKET_UTIL_ATOMIC
            ++atomic_out;
#endif
        }
    };
};

class ITcpPkgParser {
    public:
        ITcpPkgParser(){}
        virtual ~ITcpPkgParser(){}

    public:
        virtual int GetPkgLen(const char* buf, size_t buf_len) = 0;
};

// tcp util
class TcpSocketUtil {
    public:
        TcpSocketUtil();
        ~TcpSocketUtil();

    public:
        int Init(ITcpPkgParser* tcp_pkg_parser_imp, size_t buff_size);
        void Fini();

        void Reset();

    public:
        int Connect(const char* url, size_t timeout);
        void Close();

        // 纯异步模式接口
        int ConnectNonBlock(const char* url);
        int CheckNonBlock();

    public:
        int Send();
        int Recv();

    public:
        int PushToSendQ(const char* buf, size_t buf_len);
        int PeekFromRecvQ(const char** buf, int* buf_len);
        int PopFromRecvQ();

    public:
        bool HasNewPkg();

    public:
        inline TSOCKET socket_fd() const {
            return fd_;
        }

        inline int socket_state() const {
            return socket_state_;
        }

        inline size_t GetRecvBufLen() const {
            return recv_buf_.Datalen();
        }

        inline size_t GetSendBufLen() const {
            return send_buf_.Datalen();
        }

        inline const char* url() const {
            return url_.c_str();
        }

        inline bool Closed() const {
            return (fd_ < 0) ? true : false;
        }

        inline size_t last_reqtime() const {
            return last_request_timestamp_;
        }

        inline size_t last_resptime() const {
            return last_response_timestamp_;
        }

        // 通过此接口控制发送方向
        inline bool Sendable() const {
            return (NET_CONNECTED == socket_state_) ? true : false;
        }

    private:
        int socket_state_;  // socket状态：比如网络异常、连接已被关闭
        std::string url_;
        TSOCKET fd_;
        size_t last_request_timestamp_;
        size_t last_response_timestamp_;
        ITcpPkgParser* tcp_pkg_parser_imp_;

    private:
        RingBuffer send_buf_;
        RingBuffer recv_buf_;
};

}


#endif




