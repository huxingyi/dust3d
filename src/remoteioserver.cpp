#include <QNetworkConfigurationManager>
#include <QSettings>
#include <QAbstractSocket>
#include <QTcpSocket>
#include "remoteioserver.h"
#include "version.h"
#include "remoteioconnection.h"

RemoteIoServer::RemoteIoServer(int listenPort) :
    m_listenPort(listenPort)
{
    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        // Get saved network configuration
        QSettings settings;
        settings.beginGroup(QLatin1String("QtNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) !=
            QNetworkConfiguration::Discovered) {
            config = manager.defaultConfiguration();
        }

        m_networkSession = new QNetworkSession(config, this);
        connect(m_networkSession, &QNetworkSession::opened, this, &RemoteIoServer::sessionOpened);

        m_networkSession->open();
    } else {
        sessionOpened();
    }
}

RemoteIoServer::~RemoteIoServer()
{
    delete m_currentConnection;
    delete m_tcpServer;
    delete m_networkSession;
}

void RemoteIoServer::sessionOpened()
{
    // Save the used configuration
    if (m_networkSession) {
        QNetworkConfiguration config = m_networkSession->configuration();
        QString id;
        if (config.type() == QNetworkConfiguration::UserChoice)
            id = m_networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
        else
            id = config.identifier();

        QSettings settings;
        settings.beginGroup(QLatin1String("QtNetwork"));
        settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
        settings.endGroup();
    }

    m_tcpServer = new QTcpServer(this);
    if (!m_tcpServer->listen(QHostAddress::LocalHost, m_listenPort)) {
        qDebug() << "Unable to listen on remote io port, error:" << m_tcpServer->errorString();
        return;
    }
    
    connect(m_tcpServer, &QTcpServer::newConnection, this, &RemoteIoServer::handleConnection);

    qDebug() << "Remote io listen on port:" << m_tcpServer->serverPort();
}

void RemoteIoServer::handleConnection()
{
    QTcpSocket *clientSocket = m_tcpServer->nextPendingConnection();
    if (nullptr == clientSocket)
        return;
    
    if (nullptr != m_currentConnection) {
        delete m_currentConnection;
        m_currentConnection = nullptr;
    }
    m_currentConnection = new RemoteIoConnection(clientSocket);
}
