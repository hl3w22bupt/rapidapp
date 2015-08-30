#include "connector_client_api_imp.h"
#include "connector_client_api.h"
//#include "utils/tcp_socket.h"
//#include <boost/thread.hpp>
//#include <boost/bind.hpp>
//#include <iostream>
#include <cassert>

namespace hmoon_connector_api {

////////////////////////////////////////////////
////////////// Connector Protocol //////////////
////////////////////////////////////////////////
ConnectorClientProtocol::ConnectorClientProtocol() : imp_(new ConnectorClientProtocolImp())
{}

ConnectorClientProtocol::~ConnectorClientProtocol()
{}

int ConnectorClientProtocol::Start(IProtocolEventListener* protocol_evlistener,
                                   ILoggable* logger)
{
    assert(imp_ != NULL);
    return imp_->Start(protocol_evlistener, logger);
}

// 停止服务
int ConnectorClientProtocol::Terminate()
{
    assert(imp_ != NULL);
    return imp_->Terminate();
}

// 恢复服务
int ConnectorClientProtocol::Resume()
{
    assert(imp_ != NULL);
    return imp_->Resume();
}

// 驱动异步事件
int ConnectorClientProtocol::Update()
{
    assert(imp_ != NULL);
    return imp_->Update();
}

// 发送消息
int ConnectorClientProtocol::PushMessage(const char* data, size_t size)
{
    assert(imp_ != NULL);
    return imp_->PushMessage(data, size);
}

// 接收消息
int ConnectorClientProtocol::PeekMessage(const char** buf_ptr, int* buflen_ptr)
{
    assert(imp_ != NULL);
    return imp_->PeekMessage(buf_ptr, buflen_ptr);
}

int ConnectorClientProtocol::PopMessage()
{
    assert(imp_ != NULL);
    return imp_->PopMessage();
}

///////////////////////////////////////////////////////////////////
////////////// Connector Protocol With Single Thread //////////////
///////////////////////////////////////////////////////////////////
ConnectorClientProtocolThread::ConnectorClientProtocolThread() : imp_(new ConnectorClientProtocolThreadImp())
{}

ConnectorClientProtocolThread::~ConnectorClientProtocolThread()
{}

int ConnectorClientProtocolThread::StartThread(IProtocolEventListener* protocol_evlistener,
                                               IWorkerThreadListener* thread_listener,
                                               ILoggable* logger)
{
    assert(imp_ != NULL);
    return imp_->StartThread(protocol_evlistener, thread_listener, logger);
}

int ConnectorClientProtocolThread::TerminateThread()
{
    assert(imp_ != NULL);
    return imp_->TerminateThread();
}

int ConnectorClientProtocolThread::PushMessageToSendQ(const char* data,
                                                      size_t size)
{
    assert(imp_ != NULL);
    return imp_->PushMessageToSendQ(data, size);
}

int ConnectorClientProtocolThread::PopMessageFromRecvQ(char* buf_ptr,
                                                       size_t* buflen_ptr)
{
    assert(imp_ != NULL);
    return imp_->PopMessageFromRecvQ(buf_ptr, buflen_ptr);
}

}
