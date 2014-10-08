#include "tcp_socket.h"

#define TCP_SOCKET_PORT_SEP ':'
#define TCP_SOCKET_ADDR_LEN 128
#define TCP_SOCKET_URL_LEN 256

#define TCP_SOCKET_ADDR_SEP "://"
#define TCP_SOCKET_PROTO_TCP "tcp"

#define TCP_SOCKET_STR_NUMBER "1234567890."

#define MS_TO_TV(tv, ms)                \
do                                      \
{                                       \
    (tv).tv_sec = (ms)/1000;            \
    (tv).tv_usec = ((ms) % 1000)*1000;  \
} while(0)

typedef struct AddrPair {
   struct sockaddr_in stSockAdd;
   unsigned short wPort;
} TSOCKETPAIR;

#if !defined(_WIN32) && !defined(_WIN64) && defined(_LINUX)
#include <sys/epoll.h>
int tcpsocket_socket_epoll_check(TSOCKET a_iSock, int a_iEvent, int a_iTimeout)
{
    int iRet;
    int iEpollFd;
    struct epoll_event e ;
    struct epoll_event *events = NULL;

    iEpollFd = epoll_create(a_iSock + 1);

    if (0 > iEpollFd)
    {
        return -1;
    }

    memset(&e, 0, sizeof(e));
    e.events = a_iEvent;
    e.data.fd = a_iSock;

    iRet = epoll_ctl(iEpollFd, EPOLL_CTL_ADD, a_iSock, &e);
    if (0 == iRet)
    {
        events = (struct epoll_event*)malloc(sizeof(struct epoll_event)*(a_iSock+1));
        if (NULL == events)
        {
            iRet = -2;
        }
    }

    if (0 == iRet)
    {
        iRet = epoll_wait(iEpollFd, events, a_iSock+1, a_iTimeout);
        if (0 > iRet)
        {
            iRet = -3;
        } else if (0 == iRet)
        {
            iRet = 1;   //timeout
        }
        else
        {
            iRet = 0;//have ready
        }
    }

    if (NULL != events)
    {
        free(events);
    }

    close(iEpollFd);

    return iRet;
}
#endif

#if defined(_WIN32) || defined(_WIN64)
int tcpsocket_inet_aton(const char* cp, struct in_addr* inp)
{
    unsigned long ulAddr;
    if (NULL == cp || NULL == inp)
    {
        return 0;
    }

    ulAddr = inet_addr(cp);
    if (INADDR_NONE == ulAddr)
    {
        inp->S_un.S_addr = ulAddr;
        return 0;
    }

    inp->S_un.S_addr = ulAddr;
    return 1;
}
#else
#define tcpsocket_inet_aton inet_aton
#endif

int g_iConnectBlockingTimeout = 3000;     // default 3s

int tcpsocket_socket_connect(TSOCKET a_iSock, struct sockaddr* a_pstAddr,
                           int a_iLen);
int tcpsocket_check_connect(TSOCKET a_iSock, int a_iTimeout);

int tcpsocket_is_number(const char* a_pszHost)
{
	size_t iSize;

	iSize =	strspn(a_pszHost, TCP_SOCKET_STR_NUMBER);

	if(iSize == strlen(a_pszHost))
		return 1;
	else
		return 0;
}

int tcpsocket_str2inet(const char* a_pszAddr, TSOCKETPAIR* a_pstSocketPair)
{
	char szHost[TCP_SOCKET_ADDR_LEN];
	struct hostent* pstHostent=NULL;
	char* pszPort;

    if (NULL == a_pstSocketPair)
        return -1;

    memset(&a_pstSocketPair->stSockAdd, 0, sizeof(a_pstSocketPair->stSockAdd));
	a_pstSocketPair->stSockAdd.sin_family =	AF_INET;

	pszPort	= strchr(a_pszAddr, TCP_SOCKET_PORT_SEP);
	if(pszPort)
	{
		if(pszPort - a_pszAddr >= TCP_SOCKET_ADDR_LEN)
			return -1;

		memcpy(szHost, a_pszAddr, pszPort - a_pszAddr);
		szHost[pszPort - a_pszAddr]	= '\0';

		pszPort++;
		a_pstSocketPair->wPort = (unsigned short)atoi(pszPort);
	}
	else
	{
		if(strlen(a_pszAddr) >= TCP_SOCKET_ADDR_LEN)
			return -1;

		strcpy(szHost, a_pszAddr);
		a_pstSocketPair->wPort = 0;
	}

	a_pstSocketPair->stSockAdd.sin_port = htons(a_pstSocketPair->wPort);

	if(tcpsocket_is_number(szHost)) /* it is a numbering address. */
	{
		if (0 == tcpsocket_inet_aton(szHost,&a_pstSocketPair->stSockAdd.sin_addr))
		{
			return -1;
		}
	}
	else
	{
		pstHostent = gethostbyname(szHost);
		if(NULL == pstHostent)
        {
			return -1;
        }

		a_pstSocketPair->stSockAdd.sin_addr = *(struct in_addr*)pstHostent->h_addr_list[0];
	}

	return 0;
}

int tcpsocket_check_connect_nonblock(TSOCKET a_iSock, struct sockaddr_in* a_pstSin, int a_iTimeout)
{
    int iRet = 0;
    if (a_iSock < 0 || NULL == a_pstSin || a_iTimeout < 0)
        return -1;

    // set nonblock
    tcpsocket_set_nonblock(a_iSock, 1);

    // connect
    iRet = tcpsocket_socket_connect(a_iSock, (struct sockaddr*)a_pstSin, sizeof(*a_pstSin));
    if (iRet < 0)
        return iRet;

    // check connect succ or not
    return tcpsocket_check_connect(a_iSock, a_iTimeout);
}

int tcpsocket_check_connect_block(TSOCKET a_iSock, struct sockaddr_in* a_pstSin, int a_iTimeout)
{
    struct timeval timeout;
    socklen_t len = sizeof(timeout);
    int iRet = 0;

    if (a_iSock < 0 || NULL == a_pstSin || a_iTimeout < 0)
        return -1;

    // setsockopt timeout
    MS_TO_TV(timeout, a_iTimeout);
    setsockopt(a_iSock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, len);

    // connect blockingly
    iRet = tcpsocket_socket_connect(a_iSock, (struct sockaddr*)a_pstSin, sizeof(*a_pstSin));
    if (iRet != 0)
        return iRet;

    // set nonblock
    tcpsocket_set_nonblock(a_iSock, 1);

    return 0;
}

int tcpsocket_set_sendbuff(TSOCKET a_iSock, int a_iSize)
{
    if (a_iSock < 0)
    {
        return -1;
    }

	return setsockopt(a_iSock, SOL_SOCKET, SO_SNDBUF, (void*)&a_iSize, sizeof(a_iSize));
}

int tcpsocket_set_recvbuff(TSOCKET a_iSock, int a_iSize)
{
    if (a_iSock < 0)
    {
        return -1;
    }

	return setsockopt(a_iSock, SOL_SOCKET, SO_RCVBUF, (void*)&a_iSize, sizeof(a_iSize));
}

int tcpsocket_set_nodelay(TSOCKET a_iSock)
{
    int iFlag = 1;
    if (a_iSock < 0)
    {
        return -1;
    }

    return setsockopt(a_iSock, IPPROTO_TCP, TCP_NODELAY,
                      (const char *)&iFlag, sizeof(iFlag));
}

int tcpsocket_set_nonblock(TSOCKET a_iSock, int a_iIsNonblocking)
{
#if !defined(_WIN32) && !defined(_WIN64)
	int iFlags = fcntl(a_iSock, F_GETFL, 0);

	if(a_iIsNonblocking)
		iFlags |= O_NONBLOCK | O_ASYNC;
	else
		iFlags &= ~(O_NONBLOCK | O_ASYNC);

	return fcntl(a_iSock, F_SETFL, iFlags);
#else
	unsigned long lParam = a_iIsNonblocking;

	return ioctlsocket(a_iSock, FIONBIO, &lParam);
#endif
}

TSOCKET tcpsocket_open(const char* a_pszUri)
{
    // check uri, support TCP only
    if (a_pszUri != NULL)
    {
        char* pszAddr = strstr(a_pszUri, TCP_SOCKET_ADDR_SEP);
        if (pszAddr != NULL)
        {
            int iLen = pszAddr - a_pszUri;
            if (strncmp(a_pszUri, TCP_SOCKET_PROTO_TCP, iLen) != 0 ||
                strlen(TCP_SOCKET_PROTO_TCP) != iLen)
            {
                return -1;
            }
        }
    }

    return socket(PF_INET, SOCK_STREAM, 0);
}

void tcpsocket_set_connect_block_timeout(int a_iTimeout)
{
    if (a_iTimeout < 0)
        return;

    g_iConnectBlockingTimeout = a_iTimeout;
}

int tcpsocket_socket_connect(TSOCKET a_iSock, struct sockaddr* a_pstAddr, int a_iLen)
{
    int iErrCode = 0;

    if (0 == connect(a_iSock, a_pstAddr, a_iLen))
    {
        return 0;
    }

    iErrCode = socketerrno;
    if (SOCKETEISCONN == iErrCode)
        return 0;
    else if (SOCKETEINPROGRESS == iErrCode || SOCKETEALREADY == iErrCode)
        return TCP_SOCKET_INPROGRESS;
    else
        return -1;
}

#if !defined(_WIN32) && !defined(_WIN64)
int tcpsocket_check_connect(TSOCKET a_iSock, int a_iTimeout)
{
	fd_set wrfs;
	struct timeval tv;
	int iRet = 0;
	int iError = 0;
	socklen_t iLen;

    if (a_iTimeout < 0)
        return -1;

    if (a_iSock < FD_SETSIZE) //select
    {
        FD_ZERO(&wrfs);
        FD_SET(a_iSock, &wrfs);

        MS_TO_TV(tv, a_iTimeout);

        iRet = select((int)(a_iSock + 1), NULL, &wrfs, NULL, &tv);
        if (0 > iRet)
            return -1;
        else if (0 == iRet)
            return TCP_SOCKET_TIMEOUT;

        iError = 0;
        iLen = sizeof(iLen);
        getsockopt(a_iSock, SOL_SOCKET, SO_ERROR, (char*)&iError, &iLen);
        if(0 == iError)
            return 0;
        else
            return -1;
    }

    return 0;
}
#else
int tcpsocket_check_connect(TSOCKET a_iSock, int a_iTimeout)
{
	fd_set wrfs;
	struct timeval tv;
	int iRet = 0;
	fd_set exceptfs;

    if (a_iTimeout < 0)
        return -1;

    FD_ZERO(&wrfs);
    FD_SET(a_iSock, &wrfs);
    FD_ZERO(&exceptfs);
    FD_SET(a_iSock, &exceptfs);

    MS_TO_TV(tv, a_iTimeout);

    iRet = select((int)(a_iSock + 1), NULL, &wrfs, &exceptfs, &tv);

    if (0 > iRet)
        return -1;
    else if(0 == iRet)
        return TCP_SOCKET_TIMEOUT;

    if (FD_ISSET(a_iSock, &exceptfs))
        return -1;
    else
        return 0;
}
#endif

// 最大阻塞时间timeout
TSOCKET tcpsocket_connect(const char* a_pszUri, int a_iTimeout)
{
    int iRet = 0;
    const char* pszAddr = NULL;
    TSOCKETPAIR stSockPair;
    TSOCKET iSock = -1;

    if (NULL == a_pszUri)
        return -1;

    // open socket
    iSock = tcpsocket_open(a_pszUri);
    if (iSock < 0)
        return iSock;

    // parse url
    pszAddr = strstr(a_pszUri, TCP_SOCKET_ADDR_SEP);
    if (NULL == pszAddr)
        pszAddr = a_pszUri;
    else
        pszAddr += strlen(TCP_SOCKET_ADDR_SEP);

    iRet = tcpsocket_str2inet(pszAddr, &stSockPair);
    if (iRet != 0)
        return -1;

#if !defined(_WIN32) && !defined(_WIN64)        // unix-like system
    // 大于FD_SETSIZE的，采用非阻塞超时模式去connect
    if (iSock < FD_SETSIZE)
    {
        iRet = tcpsocket_check_connect_nonblock(iSock, &stSockPair.stSockAdd, a_iTimeout);
        if (iRet != 0)
            return -1;
    }
    else
    {
        iRet = tcpsocket_check_connect_block(iSock, &stSockPair.stSockAdd, a_iTimeout);
        if (iRet != 0)
            return -1;
    }
#else
    iRet = tcpsocket_check_connect_nonblock(iSock, &stSockPair.stSockAdd, a_iTimeout);
    if (iRet != 0)
        return -1;
#endif

    return iSock;
}

// 无timeout阻塞
TSOCKET tcpsocket_connect_nonblock(const char* a_pszUri)
{
    int iRet = 0;
    const char* pszAddr = NULL;
    TSOCKETPAIR stSockPair;
    TSOCKET iSock = -1;

    if (NULL == a_pszUri)
        return -1;

    // open socket
    iSock = tcpsocket_open(a_pszUri);
    if (iSock < 0)
        return iSock;

    // parse url
    pszAddr = strstr(a_pszUri, TCP_SOCKET_ADDR_SEP);
    if (NULL == pszAddr)
        pszAddr = a_pszUri;
    else
        pszAddr += strlen(TCP_SOCKET_ADDR_SEP);

    iRet = tcpsocket_str2inet(pszAddr, &stSockPair);
    if (iRet != 0)
        return -1;

#if !defined(_WIN32) && !defined(_WIN64)        // unix-like system
    // 大于FD_SETSIZE的，采用非阻塞超时模式去connect
    if (iSock < FD_SETSIZE)
    {
        // set nonblock
        tcpsocket_set_nonblock(iSock, 1);

        // connect
        iRet = tcpsocket_socket_connect(iSock, (struct sockaddr*)&stSockPair.stSockAdd,
                                      sizeof(stSockPair.stSockAdd));
        if (iRet < 0)
            return iRet;
    }
    else
    {
        struct timeval timeout;
        socklen_t len = sizeof(timeout);

        // setsockopt timeout
        MS_TO_TV(timeout, g_iConnectBlockingTimeout);
        setsockopt(iSock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, len);

        // connect blockingly
        iRet = tcpsocket_socket_connect(iSock, (struct sockaddr*)&stSockPair.stSockAdd,
                                      sizeof(stSockPair.stSockAdd));
        if (iRet != 0)
            return -1;

        // set nonblock
        tcpsocket_set_nonblock(iSock, 1);
    }
#else
    // set nonblock
    tcpsocket_set_nonblock(iSock, 1);

    // connect
    iRet = tcpsocket_socket_connect(iSock, (struct sockaddr*)&stSockPair.stSockAdd,
                                  sizeof(stSockPair.stSockAdd));
    if (iRet < 0)
        return iRet;
#endif

    return iSock;
}

int tcpsocket_close(TSOCKET a_iSock)
{
#if defined (_WIN32) || defined (_WIN64)
	return closesocket(a_iSock);
#else
	return close(a_iSock);
#endif
}

int tcpsocket_recv(TSOCKET a_iSock, char* a_pszBuff, int a_iLen, int a_iTimeout)
{
    int iRecv = 0;

    if (a_iSock < 0 || NULL == a_pszBuff || a_iLen <= 0)
    {
        return TCP_SOCKET_EARGUMENT;
    }

#if !defined(_WIN32) && !defined(_WIN64)
    if (a_iTimeout > 0 && a_iSock < FD_SETSIZE)
#else
    if (a_iTimeout > 0)
#endif
    {
        fd_set fds;
        struct timeval tv;
        int iRet = 0;
        FD_ZERO(&fds);
        FD_SET(a_iSock, &fds);

        tv.tv_sec = a_iTimeout / 1000;
        tv.tv_usec = (a_iTimeout % 1000)*1000;

        // select 返回0，可能是超时，也可能是对端主动关闭连接
        // 需要进一步调用recv来确定
        iRet = select((int)(a_iSock + 1), &fds, NULL, NULL, &tv);
        if(iRet < 0)
        {
            return TCP_SOCKET_EXCEPTION; // select异常
        }
    }
#if !defined(_WIN32) && !defined(_WIN64) && defined(_LINUX)
	else if(a_iTimeout > 0 && a_iSock >= FD_SETSIZE)
	{
		int iRet = tcpsocket_socket_epoll_check(a_iSock, EPOLLIN, a_iTimeout);
		if (iRet != 0)
		{
            if (1 == iRet)
            {
                return TCP_SOCKET_EAGAIN;
            }
            else
            {
                return TCP_SOCKET_EXCEPTION; // recv 异常
            }
		}
	}
#endif

    iRecv = recv(a_iSock, a_pszBuff, a_iLen, 0);
    if (iRecv < 0)
    {
#if !defined(_WIN32) && !defined(_WIN64)
        if (SOCKETEWOULDBLOCK == errno || SOCKETEAGAIN == errno)
#else
        if (SOCKETEWOULDBLOCK == WSAGetLastError() || SOCKETEAGAIN == WSAGetLastError())
#endif
        {
            return TCP_SOCKET_EAGAIN; // 超时(如果timeout非0的话)
        }

        return TCP_SOCKET_EXCEPTION; // recv 异常
    }
    else if(0 == iRecv)
    {
        return TCP_SOCKET_ECLOSED; // 对端主动关闭连接
    }

    return iRecv;
}


int tcpsocket_send(TSOCKET a_iSock, const char* a_pszBuff, int a_iLen, int a_iTimeout)
{
    int iRet;
    struct timeval tv;
    struct timeval* pstTV;


#if !defined(_WIN32) && !defined(_WIN64)
    if(a_iTimeout > 0 && a_iSock < FD_SETSIZE)
#else
    if(a_iTimeout > 0)
#endif
    {
        fd_set  fds;
        FD_ZERO(&fds);
        FD_SET(a_iSock, &fds);

        pstTV =	&tv;
        MS_TO_TV(tv, a_iTimeout);

        iRet = select((int)(a_iSock + 1), NULL, &fds, NULL, pstTV);

        if(iRet < 0)
            return -1;
        else if(0 == iRet)
            return 0;
    }

#if !defined(_WIN32) && !defined(_WIN64) && defined(_LINUX)
	else if(a_iTimeout > 0 && a_iSock >= FD_SETSIZE)
	{
		iRet = tcpsocket_socket_epoll_check(a_iSock, EPOLLOUT, a_iTimeout);
		if (iRet != 0)
		{
			return -1;
		}
	}
#endif

#if !defined(_WIN32) && !defined(_WIN64) && !defined(__IOS__)
    iRet = send(a_iSock, a_pszBuff, a_iLen, MSG_NOSIGNAL);
#else
    iRet = send(a_iSock, a_pszBuff, a_iLen, 0);
#endif
    if(iRet < 0)
    {
        if(SOCKETEWOULDBLOCK == socketerrno || SOCKETEAGAIN == socketerrno)
        {
            return 0;
        }

        return -2;
    }
    else if(0 == iRet)
    {
        return -3;
    }
    else
    {
        return iRet;
    }
}

#if !defined(_WIN32) && !defined(_WIN64)
#include <signal.h>
#endif

void tcpsocket_ignore_pipe(void)
{
#if defined(_WIN32) || defined(_WIN64)
    return;
#else
    struct sigaction act;

    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;

    sigaction(SIGPIPE, &act, NULL);
#endif
}

