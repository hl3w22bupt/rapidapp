#ifndef _CONNECTOR_SERVER_API_H_
#define _CONNECTOR_SERVER_API_H_

namespace hmoon_connector_api {

class IConnListener {
    public:
        IConnListener() {}
        virtual ~IConnListener() {}

    public:
        virtual int OnConnStart();
        virtual int OnConnStop();
        virtual int OnConnResume();
};

class ConnectorServerApi {
    public:
        ConnectorServerApi();
        virtual ~ConnectorServerApi();

    public:
        int Init(IConnListener* conn_listener);
        void CleanUp();

        int StopConn(uint32_t fd, uint64_t nid, uint32_t sid);

    private:
        IConnListener* conn_listener_;
};

}

#endif
