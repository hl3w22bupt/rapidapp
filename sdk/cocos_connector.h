#ifndef _COCOS_CONNECTOR_H_
#define _COCOS_CONNECTOR_H_

#include "cocos2d.h"

class ConnectorApi : public cocos2d::Ref {
    public:
        bool init() {return true;};
        CREATE_FUNC(ConnectorApi);

    public:
        static ConnectorApi* getInstance() {
            static ConnectorApi api;
            return &api;
        }

        int Start();
        void Stop();
        int Resume();

        int Send(const std::string& bin);
        int Recv(std::string& bin);
};

#endif
