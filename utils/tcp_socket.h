#ifndef TCP_SOCKET_H_
#define TCP_SOCKET_H_

#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <sys/socket.h>
	#include <sys/un.h>
    #include <netdb.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <errno.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <stdlib.h>
#endif

#if defined(_WIN32) || defined(_WIN64)

	#define socketerrno				WSAGetLastError()
	#define SOCKETEWOULDBLOCK		WSAEWOULDBLOCK
	#define SOCKETEAGAIN			WSAEWOULDBLOCK
    #define SOCKETEINPROGRESS       WSAEWOULDBLOCK
	#define SOCKETINTR              WSAEINTR
    #define SOCKETECONNRESET        WSAECONNRESET
    #define SOCKETEISCONN           WSAEISCONN
    #define SOCKETEALREADY          WSAEALREADY

    typedef SOCKET TSOCKET;
#else

	#define socketerrno				errno
	#define SOCKETEWOULDBLOCK		EWOULDBLOCK
	#define SOCKETEAGAIN			EAGAIN
    #define SOCKETEINPROGRESS       EINPROGRESS
	#define SOCKETINTR              EINTR
    #define SOCKETECONNRESET        ECONNRESET
    #define SOCKETEISCONN           EISCONN
    #define SOCKETEALREADY          EALREADY
	typedef int TSOCKET;
    #define INVALID_SOCKET  -1
#endif

#define IN
#define OUT
#define INOUT

enum {
    TCP_SOCKET_EARGUMENT   = -1,
    TCP_SOCKET_EXCEPTION   = -2,
    TCP_SOCKET_EAGAIN      = -3,
    TCP_SOCKET_ECLOSED     = -4,
    TCP_SOCKET_NO_ERR      = 0,
    TCP_SOCKET_TIMEOUT     = 1,
    TCP_SOCKET_INPROGRESS  = 2,
};

#ifdef __cplusplus
extern "C"
{
#endif

int tcpsocket_set_nonblock(IN TSOCKET a_iSock, IN int a_iIsNonblocking);

int tcpsocket_set_recvbuff(IN TSOCKET a_iSock, IN int a_iSize);
int tcpsocket_set_sendbuff(IN TSOCKET a_iSock, IN int a_iSize);
int tcpsocket_set_nodelay(IN TSOCKET a_iSock);

void tcpsocket_set_connect_block_timeout(int a_iTimeout);

TSOCKET tcpsocket_open(IN const char* a_pszUri);
int tcpsocket_close(IN TSOCKET a_iSock);

int tcpsocket_str2sockin(const char* a_pszUri,
                       struct sockaddr_in* a_stSockIn);

TSOCKET tcpsocket_connect(IN const char* a_pszUri, IN int a_iTimeout);
TSOCKET tcpsocket_connect_nonblock(const char* a_pszUri);
int tcpsocket_check_connect(TSOCKET a_iSock, int a_iTimeout);

int tcpsocket_send(IN TSOCKET a_iSock, IN const char* a_pszBuff, IN int a_iLen,
                     IN int a_iTimeout);
int tcpsocket_recv(IN TSOCKET a_iSock, OUT char* a_pszBuff, IN int a_iLen,
                     IN int a_iTimeout);

void tcpsocket_ignore_pipe(void);
#ifdef __cplusplus
}
#endif

#endif

