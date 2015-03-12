#ifndef _CONNECTOR_SERVER_API_H_
#define _CONNECTOR_SERVER_API_H_

#include "../server.pb.h"

namespace hmoon_connector_api {

class IConnListener {
    public:
        IConnListener() {}
        virtual ~IConnListener() {}

    public:
        virtual int OnConnStart() {return 0;}
        virtual int OnConnStop() {return 0;}
        virtual int OnConnResume() {return 0;}

        virtual int OnData() = 0;
        virtual int SendToConn(const char* data, size_t len) = 0;
};

class ConnectorServerApi {
    public:
        ConnectorServerApi();
        virtual ~ConnectorServerApi();

    public:
        int Init(IConnListener* conn_listener);
        void CleanUp();

        int Dispatch(const char* data, size_t len);

        int StopConn(uint32_t fd, uint64_t nid, uint32_t sid);

    private:
        IConnListener* conn_listener_;
};

}

#endif
