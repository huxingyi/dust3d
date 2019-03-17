#ifndef DUST3D_REMOTE_IO_SERVER_H
#define DUST3D_REMOTE_IO_SERVER_H
#include <QObject>
#include <QTcpServer>
#include <QNetworkSession>
#include "remoteioconnection.h"

class RemoteIoServer : public QObject
{
    Q_OBJECT

public:
    explicit RemoteIoServer(int listenPort);
    ~RemoteIoServer();
    
private slots:
    void sessionOpened();
    void handleConnection();
    
private:
    int m_listenPort = 0;
    QTcpServer *m_tcpServer = nullptr;
    QNetworkSession *m_networkSession = nullptr;
    RemoteIoConnection *m_currentConnection = nullptr;
};

#endif
