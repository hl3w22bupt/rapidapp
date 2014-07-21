#ifndef CONNECTOR_SESSION_H_
#define CONNECTOR_SESSION_H_

#include "rapidapp_easy_net.h"
#include <tr1/unordered_map>

using namespace rapidapp;

enum SessionState {
    STATE_INIT = 0,
    STATE_AUTH = 1,
    STATE_OK   = 2,
};

class ConnectorSession {
    public:
        ConnectorSession();
        ~ConnectorSession();

    public:
        int Init(EasyNet* net);
        void CleanUp();

    public:
        void ChangeState(int status_code);

    public:
        inline int state() const {
            return state_;
        }

    private:
        void SetChannelID(int channel_id);

    private:
        int state_;         // 状态机状态
        EasyNet* net_stub_; // 网络连接stub
        int channel_id_;    // 后端服务器channel id

        friend class ConnectorSessionMgr;
};

#endif
