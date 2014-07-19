#include "connector_session.h"
#include <glog/logging.h>

const int kDefaultOverloadLimit = 1024;

ConnectorSession::ConnectorSession() :
    state_(STATE_INIT), net_stub_(NULL), channel_id_(-1)
{}

ConnectorSession::~ConnectorSession()
{
    CleanUp();
}

int ConnectorSession::Init(EasyNet* net)
{
    if (NULL == net)
    {
        return -1;
    }

    net_stub_ = net;
    state_ = STATE_INIT;
    return 0;
}

void ConnectorSession::CleanUp()
{
    state_ = STATE_INIT;
}

void ConnectorSession::ChangeState(int status_code)
{
    switch(state_)
    {
        case STATE_INIT:
            {
                break;
            }
        case STATE_AUTH:
            {
                break;
            }
        case STATE_OK:
            {
                break;
            }
        default:
            break;
    }
}

void ConnectorSession::SetChannelID(int channel_id)
{
    channel_id_ = channel_id;
}
