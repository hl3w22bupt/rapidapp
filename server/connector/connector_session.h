#ifndef CONNECTOR_SESSION_H_
#define CONNECTOR_SESSION_H_

#include "rapidapp_easy_net.h"
#include <tr1/unordered_map>

class ConnectorSession;
typedef std::tr1::unordered_map<int, ConnectorSession*> SessionPool;

using namespace rapidapp;

enum SessionState {
    STATE_INIT = 0,
    STATE_AUTH = 1,
    STATE_OK   = 2,
};

class ConnectorSessionMgr;
class ConnectorSession {
    public:
        ConnectorSession();
        ~ConnectorSession();

    public:
        int Init(EasyNet* net);
        void CleanUp();

    public:
        void ChangeState(int status_code);

    private:
        void SetChannelID(int channel_id);

    private:
        int state_;         // 状态机状态
        EasyNet* net_stub_; // 网络连接stub
        int channel_id_;    // 后端服务器channel id

        friend class ConnectorSessionMgr;
};

class ConnectorSessionMgr {
    public:
        ConnectorSessionMgr();
        ConnectorSessionMgr(unsigned int overload_limit);
        ~ConnectorSessionMgr();

    public:
        ConnectorSession* CreateInstance(EasyNet* net);
        void DestroyInstance(ConnectorSession** session);

    private:
        unsigned int overload_limit_;
        SessionPool session_pool_;
};

#endif
