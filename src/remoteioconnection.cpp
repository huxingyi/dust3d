#include <QAbstractSocket>
#include <QHostAddress>
#include "remoteioconnection.h"

RemoteIoConnection::RemoteIoConnection(QTcpSocket *tcpSocket) :
    m_tcpSocket(tcpSocket)
{
    qDebug() << "Received new remote io connection from:" << m_tcpSocket->peerAddress();
    
    connect(m_tcpSocket, &QAbstractSocket::disconnected, this, [&]() {
        qDebug() << "Remote connection disconnected from:" << m_tcpSocket->peerAddress();
    });
    connect(m_tcpSocket, SIGNAL(readyRead()),this, SLOT(handleRead()));
    
    m_commandHandlers["listwindow"] = &RemoteIoConnection::commandListWindow;
    m_commandHandlers["selectwindow"] = &RemoteIoConnection::commandSelectWindow;
}

RemoteIoConnection::~RemoteIoConnection()
{
    qDebug() << "Delete remote io connection";
    
    delete m_tcpSocket;
}

void RemoteIoConnection::handleRead()
{
    m_receiveCache += m_tcpSocket->readAll();
    
    for (auto command = nextCommandFromCache();
            !command.isEmpty();
            command = nextCommandFromCache()) {
        qDebug() << "Received remote io command:" << command;
        int parametersBegin = -1;
        QString commandName = nameFromCommand(command, &parametersBegin);
        commandName = commandName.toLower();
        auto findHandler = m_commandHandlers.find(commandName);
        if (findHandler == m_commandHandlers.end()) {
            qDebug() << "Unrecognized command:" << commandName;
            continue;
        }
        QString errorMessage;
        auto response = findHandler->second(this, -1 == parametersBegin ? QByteArray() : command.mid(parametersBegin), &errorMessage);
        if (errorMessage.isEmpty())
            m_tcpSocket->write(QByteArray("+OK\r\n").toHex());
        else
            m_tcpSocket->write(("-" + errorMessage + "\r\n").toUtf8().toHex());
        m_tcpSocket->write(response.toHex());
        m_tcpSocket->write(0);
    }
}

bool RemoteIoConnection::isWhitespace(char c)
{
    return ' ' == c || '\t' == c || '\r' == c || '\n' == c;
}

QString RemoteIoConnection::nameFromCommand(const QByteArray &command, int *parametersBegin)
{
    if (nullptr != parametersBegin) {
        *parametersBegin = -1;
    }
    for (int i = 0; i < command.size(); ++i) {
        if (isWhitespace(command[i])) {
            int nameEnd = i;
            if (nullptr != parametersBegin) {
                while (isWhitespace(command[i]))
                    ++i;
                if (i < command.size())
                    *parametersBegin = i;
            }
            return command.mid(0, nameEnd);
        }
    }
    return QString(command);
}

QByteArray RemoteIoConnection::nextCommandFromCache()
{
    for (int i = 0; i < m_receiveCache.size(); ++i) {
        if ('\0' == m_receiveCache[i]) {
            auto hexBuffer = m_receiveCache.mid(0, i);
            if (0 != hexBuffer.size() % 2) {
                qDebug() << "Received invalid remote io packet with length:" << hexBuffer.size();
                return QByteArray();
            }
            auto command = QByteArray::fromHex(hexBuffer);
            m_receiveCache = m_receiveCache.mid(i + 1);
            return command;
        }
    }
    return QByteArray();
}

QByteArray RemoteIoConnection::commandListWindow(const QByteArray &parameters, QString *errorMessage)
{
    Q_UNUSED(parameters);
    Q_UNUSED(errorMessage);
    
    QByteArray response;
    for (const auto &it: DocumentWindow::documentWindows()) {
        response += it.second.toString() + QString(" ") + it.first->windowTitle().replace(" ", "%20") + "\r\n";
    }
    return response;
}

QByteArray RemoteIoConnection::commandSelectWindow(const QByteArray &parameters, QString *errorMessage)
{
    Q_UNUSED(parameters);
    Q_UNUSED(errorMessage);
    
    QByteArray response;
    if (parameters.isEmpty()) {
        *errorMessage = "Must specify window id";
        return QByteArray();
    }
    QUuid windowId = QUuid(QString(parameters));
    if (windowId.isNull()) {
        *errorMessage = "Window id is invalid:" + QString(parameters);
        return QByteArray();
    }
    for (const auto &it: DocumentWindow::documentWindows()) {
        if (it.second == windowId) {
            qDebug() << "Remote io select window:" << it.second;
            m_currentDocumentWindow = it.first;
            connect(m_currentDocumentWindow, &DocumentWindow::uninialized, this, [&]() {
                if (sender() == m_currentDocumentWindow) {
                    m_currentDocumentWindow = nullptr;
                    qDebug() << "Selected window destroyed";
                }
            });
            return QByteArray();
        }
    }
    *errorMessage = "Window id not found:" + QString(parameters);
    return QByteArray();
}
