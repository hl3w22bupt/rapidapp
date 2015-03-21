#ifndef CONNECTOR_SESSION_H_
#define CONNECTOR_SESSION_H_

#include "rapidapp_easy_net.h"
#include "rapidapp_framework.h"
#include "./client.pb.h"
#include <tr1/unordered_map>

using namespace rapidapp;

enum SessionState {
    STATE_INIT    = 0,
    STATE_AUTH    = 1,
    STATE_SYNING  = 2,
    STATE_SYNACK  = 3,
    STATE_OK      = 4,
};

enum OperationCode {
    OPERATION_NONE  = 0,
    OPERATION_CSTOP = 1,
    OPERATION_SSTOP = 2,
};

// 全局后端节点
const int kMaxBackEndNum = 32;

class BackEndSet {
    public:
        static int AddBackEnd(EasyNet* net);

    public:
        static uint32_t backend_pos_;
        static uint32_t backend_used_;
        static EasyNet* backends_[kMaxBackEndNum];
};

class ConnectorSession {
    public:
        ConnectorSession();
        ~ConnectorSession();

    public:
        int Init(EasyNet* net, IFrameWork* app_framework);
        void CleanUp();

    public:
        int DriveStateMachine();

        int ForwardUpSideMessage(uint64_t sequence, const std::string& data);
        int ForwardDownSideMessage(uint64_t sequence, const char* msg, size_t size);

        int StopBackEndSession();
        int CheckUpSideMessage(const connector_client::CSMsg& up_msg);

    public:
        inline int state() const {
            return state_;
        }

        inline EasyNet* net() {
            return net_stub_;
        }

        inline void set_sid(uint32_t sid) {
            sid_ = sid;
        }

        inline uint32_t sid() const {
            return sid_;
        }

        inline bool BeenReady() const {
            return (STATE_OK == state_);
        }

    private:
        int SerializeAndSendToFrontEnd(const connector_client::CSMsg& msg);

        int StartToBackEnd(EasyNet* back_net);
        int StopToBackEnd(EasyNet* back_net);

        int HandleKeyMaking();
        int HandleAuthRequest();

        int HandShake_StartSession();
        int HandShake_OnStartAcked();
        int HandShake_OnStopNotify();

        int SendDataToBackEnd(const char* data, size_t len);

    private:
        void SetChannelID(int channel_id);

    private:
        int state_;         // 状态机状态
        EasyNet* net_stub_; // 网络连接stub
        int channel_id_;    // 后端服务器channel id
        uint32_t sid_;

        IFrameWork* frame_stub_;   // 框架实例引用
        uint64_t last_seq_;

        /*friend class ConnectorSessionMgr;*/
};

#endif
