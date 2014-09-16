#include "connector_session.h"
#include <glog/logging.h>

const int kDefaultOverloadLimit = 1024;

ConnectorSession::ConnectorSession() :
    state_(STATE_INIT), net_stub_(NULL), channel_id_(-1), sid_(-1)
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

int ConnectorSession::DriveStateMachine()
{
    switch(state_)
    {
        case STATE_INIT:
            {
                state_ = STATE_AUTH;
                return DoAuthRequest();
                break;
            }
        case STATE_AUTH:
            {
                state_ = STATE_SYNING;
                return HandShake_StartSession();
                break;
            }
        case STATE_SYNING:
            {
                state_ = STATE_OK;
                return HandShake_OnStartAcked();
                break;
            }
        case STATE_OK:
            {
                return 0;
                break;
            }
        default:
            return -1;
    }
}

int ConnectorSession::DoAuthRequest()
{
    return 0;
}

int ConnectorSession::HandShake_StartSession()
{
    return 0;
}

int ConnectorSession::HandShake_OnStartAcked()
{
    return 0;
}

int ConnectorSession::HandShake_StopSession()
{
    return 0;
}

int ConnectorSession::HandShake_OnStopNotify()
{
    return 0;
}

void ConnectorSession::SetChannelID(int channel_id)
{
    channel_id_ = channel_id;
}
