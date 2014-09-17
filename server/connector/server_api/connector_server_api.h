#ifndef _CONNECTOR_SERVER_API_H_
#define _CONNECTOR_SERVER_API_H_

namespace hmoon_connector_api {

class ConnectorServerApi {
    public:
        ConnectorServerApi();
        virtual ~ConnectorServerApi();

    public:
        int Init();
        void CleanUp();

        int Start();
        int Stop();
        int Resume();
};

}

#endif
