#ifndef _CONNECTOR_CLIENT_API_H_
#define _CONNECTOR_CLIENT_API_H_

namespace hmoon_connector_api {

class ConnectorClientApi {
    public:
        ConnectorClientApi();
        virtual ~ConnectorClientApi();

    public:
        int Init();
        void CleanUp();

        int Start();
        int Stop();
        int Resume();
};

}

#endif
