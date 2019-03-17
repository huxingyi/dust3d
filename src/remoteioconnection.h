#ifndef REMOTE_IO_CONNECTION_H
#define REMOTE_IO_CONNECTION_H
#include <QObject>
#include <QTcpSocket>
#include <vector>
#include <QByteArray>
#include <map>
#include <QList>
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
    QList<QMetaObject::Connection> m_documentConnections;
    
    QByteArray nextCommandFromCache();
    QString nameFromCommand(const QByteArray &command, int *parametersBegin=nullptr);
    QString nextParameter(const QByteArray &parameters, int *offset);
    bool isWhitespace(char c);
    void connectDocumentSignals();
    void disconnectDocumentSignals();
    void connectDocumentWindow(DocumentWindow *window);
    void respondEventWithId(const QString &eventName, const QUuid &id);
    void respondEvent(const QString &eventName);
    
    QByteArray commandListWindow(const QByteArray &parameters, QString *errorMessage);
    QByteArray commandSelectWindow(const QByteArray &parameters, QString *errorMessage);
    QByteArray commandUndo(const QByteArray &parameters, QString *errorMessage);
    QByteArray commandRedo(const QByteArray &parameters, QString *errorMessage);
    QByteArray commandPaste(const QByteArray &parameters, QString *errorMessage);
    QByteArray commandRemoveNode(const QByteArray &parameters, QString *errorMessage);
    QByteArray commandRemoveEdge(const QByteArray &parameters, QString *errorMessage);
    QByteArray commandRemovePart(const QByteArray &parameters, QString *errorMessage);
    QByteArray commandAddNode(const QByteArray &parameters, QString *errorMessage);
};

#endif

