#ifndef REMOTE_IO_CONNECTION_H
#define REMOTE_IO_CONNECTION_H
#include <QObject>
#include <QTcpSocket>
#include <vector>
#include <QByteArray>
#include <map>
#include "documentwindow.h"

class RemoteIoConnection;
using CommandHandler = std::function<QByteArray(RemoteIoConnection *, const QByteArray&, QString *)>;

class RemoteIoConnection : QObject
{
    Q_OBJECT

public:
    explicit RemoteIoConnection(QTcpSocket *tcpSocket);
    ~RemoteIoConnection();
    
private slots:
    void handleRead();

private:
    QTcpSocket *m_tcpSocket = nullptr;
    QByteArray m_receiveCache;
    DocumentWindow *m_currentDocumentWindow = nullptr;
    std::map<QString, CommandHandler> m_commandHandlers;
    
    QByteArray nextCommandFromCache();
    QString nameFromCommand(const QByteArray &command, int *parametersBegin=nullptr);
    bool isWhitespace(char c);
    
    QByteArray commandListWindow(const QByteArray &parameters, QString *errorMessage);
    QByteArray commandSelectWindow(const QByteArray &parameters, QString *errorMessage);
};

#endif

