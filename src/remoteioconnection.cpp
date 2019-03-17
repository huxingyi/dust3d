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
    m_commandHandlers["undo"] = &RemoteIoConnection::commandUndo;
    m_commandHandlers["redo"] = &RemoteIoConnection::commandRedo;
    m_commandHandlers["paste"] = &RemoteIoConnection::commandPaste;
    m_commandHandlers["removenode"] = &RemoteIoConnection::commandRemoveNode;
    m_commandHandlers["removeedge"] = &RemoteIoConnection::commandRemoveEdge;
    m_commandHandlers["removepart"] = &RemoteIoConnection::commandRemovePart;
    m_commandHandlers["addnode"] = &RemoteIoConnection::commandAddNode;
    
    if (!DocumentWindow::documentWindows().empty()) {
        connectDocumentWindow(DocumentWindow::documentWindows().begin()->first);
    }
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
        QByteArray fullResponse;
        if (errorMessage.isEmpty())
            fullResponse += QByteArray("+OK\r\n");
        else
            fullResponse += ("-" + errorMessage + "\r\n").toUtf8();
        fullResponse += response;
        
        fullResponse = fullResponse.toHex();
        fullResponse.append((char)0);
        m_tcpSocket->write(fullResponse);
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
                while (i < command.size() && isWhitespace(command[i]))
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

QString RemoteIoConnection::nextParameter(const QByteArray &parameters, int *offset)
{
    if (*offset >= parameters.size())
        return QString();
    
    for (int i = *offset; i < parameters.size(); ++i) {
        if (isWhitespace(parameters[i])) {
            int parameterEnd = i;
            auto parameter = parameters.mid(*offset, parameterEnd);
            while (i < parameters.size() && isWhitespace(parameters[i]))
                ++i;
            *offset = i;
            return parameter;
        }
    }
    auto parameter = parameters.mid(*offset);
    *offset = parameters.size();
    return parameter;
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

void RemoteIoConnection::respondEventWithId(const QString &eventName, const QUuid &id)
{
    QByteArray fullResponse = QString("#%1 %2\r\n").arg(eventName).arg(id.toString()).toUtf8();
    fullResponse = fullResponse.toHex();
    fullResponse.append((char)0);
    m_tcpSocket->write(fullResponse);
}

void RemoteIoConnection::respondEvent(const QString &eventName)
{
    QByteArray fullResponse = QString("#%1\r\n").arg(eventName).toUtf8();
    fullResponse = fullResponse.toHex();
    fullResponse.append((char)0);
    m_tcpSocket->write(fullResponse);
}

void RemoteIoConnection::connectDocumentSignals()
{
    if (nullptr == m_currentDocumentWindow)
        return;
    
    auto document = m_currentDocumentWindow->document();
    
    m_documentConnections << connect(document, &Document::nodeAdded, this, [&](QUuid nodeId) {
        respondEventWithId("nodeadded", nodeId);
    });
    m_documentConnections << connect(document, &Document::partAdded, this, [&](QUuid partId) {
        respondEventWithId("partadded", partId);
    });
    m_documentConnections << connect(document, &Document::edgeAdded, this, [&](QUuid edgeId) {
        respondEventWithId("edgeadded", edgeId);
    });
    m_documentConnections << connect(document, &Document::partRemoved, this, [&](QUuid partId) {
        respondEventWithId("partremoved", partId);
    });
    m_documentConnections << connect(document, &Document::componentNameChanged, this, [&](QUuid componentId) {
        respondEventWithId("componentnamechanged", componentId);
    });
    m_documentConnections << connect(document, &Document::componentChildrenChanged, this, [&](QUuid componentId) {
        respondEventWithId("componentchildrenchanged", componentId);
    });
    m_documentConnections << connect(document, &Document::componentRemoved, this, [&](QUuid componentId) {
        respondEventWithId("componentremoved", componentId);
    });
    m_documentConnections << connect(document, &Document::componentAdded, this, [&](QUuid componentId) {
        respondEventWithId("componentadded", componentId);
    });
    m_documentConnections << connect(document, &Document::componentExpandStateChanged, this, [&](QUuid componentId) {
        respondEventWithId("componentexpandstatechanged", componentId);
    });
    m_documentConnections << connect(document, &Document::nodeRemoved, this, [&](QUuid nodeId) {
        respondEventWithId("noderemoved", nodeId);
    });
    m_documentConnections << connect(document, &Document::edgeRemoved, this, [&](QUuid edgeId) {
        respondEventWithId("edgeremoved", edgeId);
    });
    m_documentConnections << connect(document, &Document::nodeRadiusChanged, this, [&](QUuid nodeId) {
        respondEventWithId("noderadiuschanged", nodeId);
    });
    m_documentConnections << connect(document, &Document::nodeBoneMarkChanged, this, [&](QUuid nodeId) {
        respondEventWithId("nodebonemarkchanged", nodeId);
    });
    m_documentConnections << connect(document, &Document::nodeOriginChanged, this, [&](QUuid nodeId) {
        respondEventWithId("nodeoriginchanged", nodeId);
    });
    m_documentConnections << connect(document, &Document::edgeChanged, this, [&](QUuid edgeId) {
        respondEventWithId("edgechanged", edgeId);
    });
    m_documentConnections << connect(document, &Document::partPreviewChanged, this, [&](QUuid partId) {
        respondEventWithId("partpreviewchanged", partId);
    });
    m_documentConnections << connect(document, &Document::resultMeshChanged, this, [&]() {
        respondEvent("resultmeshchanged");
    });
    m_documentConnections << connect(document, &Document::turnaroundChanged, this, [&]() {
        respondEvent("turnaroundchanged");
    });
    m_documentConnections << connect(document, &Document::editModeChanged, this, [&]() {
        respondEvent("editmodechanged");
    });
    m_documentConnections << connect(document, &Document::skeletonChanged, this, [&]() {
        respondEvent("skeletonchanged");
    });
    m_documentConnections << connect(document, &Document::resultTextureChanged, this, [&]() {
        respondEvent("resulttexturechanged");
    });
    m_documentConnections << connect(document, &Document::postProcessedResultChanged, this, [&]() {
        respondEvent("postprocessedresultchanged");
    });
    m_documentConnections << connect(document, &Document::resultRigChanged, this, [&]() {
        respondEvent("resultrigchanged");
    });
    m_documentConnections << connect(document, &Document::rigChanged, this, [&]() {
        respondEvent("rigchanged");
    });
    m_documentConnections << connect(document, &Document::partLockStateChanged, this, [&](QUuid partId) {
        respondEventWithId("partlockstatechanged", partId);
    });
    m_documentConnections << connect(document, &Document::partVisibleStateChanged, this, [&](QUuid partId) {
        respondEventWithId("partvisiblestatechanged", partId);
    });
    m_documentConnections << connect(document, &Document::partSubdivStateChanged, this, [&](QUuid partId) {
        respondEventWithId("partsubdivstatechanged", partId);
    });
    m_documentConnections << connect(document, &Document::partDisableStateChanged, this, [&](QUuid partId) {
        respondEventWithId("partdisablestatechanged", partId);
    });
    m_documentConnections << connect(document, &Document::partXmirrorStateChanged, this, [&](QUuid partId) {
        respondEventWithId("partxmirrorstatechanged", partId);
    });
    m_documentConnections << connect(document, &Document::partDeformThicknessChanged, this, [&](QUuid partId) {
        respondEventWithId("partdeformthicknesschanged", partId);
    });
    m_documentConnections << connect(document, &Document::partDeformWidthChanged, this, [&](QUuid partId) {
        respondEventWithId("partdeformwidthchanged", partId);
    });
    m_documentConnections << connect(document, &Document::partRoundStateChanged, this, [&](QUuid partId) {
        respondEventWithId("partroundstatechanged", partId);
    });
    m_documentConnections << connect(document, &Document::partColorStateChanged, this, [&](QUuid partId) {
        respondEventWithId("partcolorstatechanged", partId);
    });
    m_documentConnections << connect(document, &Document::partCutRotationChanged, this, [&](QUuid partId) {
        respondEventWithId("partcutrotationchanged", partId);
    });
    m_documentConnections << connect(document, &Document::partCutTemplateChanged, this, [&](QUuid partId) {
        respondEventWithId("partcuttemplatechanged", partId);
    });
    m_documentConnections << connect(document, &Document::partMaterialIdChanged, this, [&](QUuid partId) {
        respondEventWithId("partmaterialidchanged", partId);
    });
    m_documentConnections << connect(document, &Document::partChamferStateChanged, this, [&](QUuid partId) {
        respondEventWithId("partchamferstatechanged", partId);
    });
    m_documentConnections << connect(document, &Document::componentCombineModeChanged, this, [&](QUuid componentId) {
        respondEventWithId("componentcombinemodechanged", componentId);
    });
    m_documentConnections << connect(document, &Document::cleanup, this, [&]() {
        respondEvent("cleanup");
    });
    m_documentConnections << connect(document, &Document::originChanged, this, [&]() {
        respondEvent("originchanged");
    });
    m_documentConnections << connect(document, &Document::xlockStateChanged, this, [&]() {
        respondEvent("xlockstatechanged");
    });
    m_documentConnections << connect(document, &Document::ylockStateChanged, this, [&]() {
        respondEvent("ylockstatechanged");
    });
    m_documentConnections << connect(document, &Document::zlockStateChanged, this, [&]() {
        respondEvent("zlockstatechanged");
    });
    m_documentConnections << connect(document, &Document::radiusLockStateChanged, this, [&]() {
        respondEvent("radiuslockstatechanged");
    });
    m_documentConnections << connect(document, &Document::checkPart, this, [&](QUuid partId) {
        respondEventWithId("checkpart", partId);
    });
    m_documentConnections << connect(document, &Document::partChecked, this, [&](QUuid partId) {
        respondEventWithId("partchecked", partId);
    });
    m_documentConnections << connect(document, &Document::partUnchecked, this, [&]() {
        respondEvent("partunchecked");
    });
    m_documentConnections << connect(document, &Document::enableBackgroundBlur, this, [&]() {
        respondEvent("enablebackgroundblur");
    });
    m_documentConnections << connect(document, &Document::disableBackgroundBlur, this, [&]() {
        respondEvent("disablebackgroundblur");
    });
    m_documentConnections << connect(document, &Document::exportReady, this, [&]() {
        respondEvent("exportready");
    });
    m_documentConnections << connect(document, &Document::uncheckAll, this, [&]() {
        respondEvent("uncheckall");
    });
    m_documentConnections << connect(document, &Document::checkNode, this, [&](QUuid nodeId) {
        respondEventWithId("checknode", nodeId);
    });
    m_documentConnections << connect(document, &Document::checkEdge, this, [&](QUuid edgeId) {
        respondEventWithId("checkedge", edgeId);
    });
    m_documentConnections << connect(document, &Document::optionsChanged, this, [&]() {
        respondEvent("optionschanged");
    });
    m_documentConnections << connect(document, &Document::rigTypeChanged, this, [&]() {
        respondEvent("rigtypechanged");
    });
    m_documentConnections << connect(document, &Document::posesChanged, this, [&]() {
        respondEvent("poseschanged");
    });
    m_documentConnections << connect(document, &Document::motionsChanged, this, [&]() {
        respondEvent("motionschanged");
    });
    m_documentConnections << connect(document, &Document::poseAdded, this, [&](QUuid poseId) {
        respondEventWithId("poseadded", poseId);
    });
    m_documentConnections << connect(document, &Document::poseRemoved, this, [&](QUuid poseId) {
        respondEventWithId("poseremoved", poseId);
    });
    m_documentConnections << connect(document, &Document::poseListChanged, this, [&]() {
        respondEvent("poselistchanged");
    });
    m_documentConnections << connect(document, &Document::poseNameChanged, this, [&](QUuid poseId) {
        respondEventWithId("posenamechanged", poseId);
    });
    m_documentConnections << connect(document, &Document::poseFramesChanged, this, [&](QUuid poseId) {
        respondEventWithId("poseframeschanged", poseId);
    });
    m_documentConnections << connect(document, &Document::poseTurnaroundImageIdChanged, this, [&](QUuid poseId) {
        respondEventWithId("poseturnaroundimageidchanged", poseId);
    });
    m_documentConnections << connect(document, &Document::posePreviewChanged, this, [&](QUuid poseId) {
        respondEventWithId("posepreviewchanged", poseId);
    });
    m_documentConnections << connect(document, &Document::motionAdded, this, [&](QUuid motionId) {
        respondEventWithId("motionadded", motionId);
    });
    m_documentConnections << connect(document, &Document::motionRemoved, this, [&](QUuid motionId) {
        respondEventWithId("motionremoved", motionId);
    });
    m_documentConnections << connect(document, &Document::motionListChanged, this, [&]() {
        respondEvent("motionlistchanged");
    });
    m_documentConnections << connect(document, &Document::motionNameChanged, this, [&](QUuid motionId) {
        respondEventWithId("motionnamechanged", motionId);
    });
    m_documentConnections << connect(document, &Document::motionClipsChanged, this, [&](QUuid motionId) {
        respondEventWithId("motionclipschanged", motionId);
    });
    m_documentConnections << connect(document, &Document::motionPreviewChanged, this, [&](QUuid motionId) {
        respondEventWithId("motionpreviewchanged", motionId);
    });
    m_documentConnections << connect(document, &Document::motionResultChanged, this, [&](QUuid motionId) {
        respondEventWithId("motionresultchanged", motionId);
    });
    m_documentConnections << connect(document, &Document::materialAdded, this, [&](QUuid materialId) {
        respondEventWithId("materialadded", materialId);
    });
    m_documentConnections << connect(document, &Document::materialRemoved, this, [&](QUuid materialId) {
        respondEventWithId("materialremoved", materialId);
    });
    m_documentConnections << connect(document, &Document::materialListChanged, this, [&]() {
        respondEvent("materiallistchanged");
    });
    m_documentConnections << connect(document, &Document::materialNameChanged, this, [&](QUuid materialId) {
        respondEventWithId("materialnamechanged", materialId);
    });
    m_documentConnections << connect(document, &Document::materialLayersChanged, this, [&](QUuid materialId) {
        respondEventWithId("materiallayerschanged", materialId);
    });
    m_documentConnections << connect(document, &Document::materialPreviewChanged, this, [&](QUuid materialId) {
        respondEventWithId("materialpreviewchanged", materialId);
    });
    m_documentConnections << connect(document, &Document::meshGenerating, this, [&]() {
        respondEvent("meshgenerating");
    });
    m_documentConnections << connect(document, &Document::postProcessing, this, [&]() {
        respondEvent("postprocessing");
    });
    m_documentConnections << connect(document, &Document::textureGenerating, this, [&]() {
        respondEvent("texturegenerating");
    });
    m_documentConnections << connect(document, &Document::textureChanged, this, [&]() {
        respondEvent("texturechanged");
    });
}

void RemoteIoConnection::disconnectDocumentSignals()
{
    for (const auto &it: m_documentConnections) {
        disconnect(it);
    }
    m_documentConnections.clear();
}

QByteArray RemoteIoConnection::commandSelectWindow(const QByteArray &parameters, QString *errorMessage)
{
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
            connectDocumentWindow(it.first);
            return QByteArray();
        }
    }
    *errorMessage = "Window id not found:" + QString(parameters);
    return QByteArray();
}

void RemoteIoConnection::connectDocumentWindow(DocumentWindow *window)
{
    if (m_currentDocumentWindow == window)
        return;
    disconnectDocumentSignals();
    m_currentDocumentWindow = window;
    if (nullptr != m_currentDocumentWindow) {
        connect(m_currentDocumentWindow, &DocumentWindow::uninialized, this, [&]() {
            if (sender() == m_currentDocumentWindow) {
                m_currentDocumentWindow = nullptr;
                qDebug() << "Selected window destroyed";
            }
        });
        connectDocumentSignals();
    }
}

#define COMMAND_PRECHECK() do {                 \
    if (nullptr == m_currentDocumentWindow) {   \
        *errorMessage = "No window selected";   \
        return QByteArray();                    \
    }                                           \
} while (false)

QByteArray RemoteIoConnection::commandUndo(const QByteArray &parameters, QString *errorMessage)
{
    COMMAND_PRECHECK();
    
    m_currentDocumentWindow->document()->undo();
    return QByteArray();
}

QByteArray RemoteIoConnection::commandRedo(const QByteArray &parameters, QString *errorMessage)
{
    COMMAND_PRECHECK();
    
    m_currentDocumentWindow->document()->redo();
    return QByteArray();
}

QByteArray RemoteIoConnection::commandPaste(const QByteArray &parameters, QString *errorMessage)
{
    COMMAND_PRECHECK();
    
    m_currentDocumentWindow->document()->paste();
    return QByteArray();
}

QByteArray RemoteIoConnection::commandRemoveNode(const QByteArray &parameters, QString *errorMessage)
{
    COMMAND_PRECHECK();
    
    if (parameters.isEmpty()) {
        *errorMessage = "Must specify node id";
        return QByteArray();
    }
    QUuid nodeId = QUuid(QString(parameters));
    if (nodeId.isNull()) {
        *errorMessage = "Node id is invalid:" + QString(parameters);
        return QByteArray();
    }
    m_currentDocumentWindow->document()->removeNode(nodeId);
    return QByteArray();
}

QByteArray RemoteIoConnection::commandRemoveEdge(const QByteArray &parameters, QString *errorMessage)
{
    COMMAND_PRECHECK();
    
    if (parameters.isEmpty()) {
        *errorMessage = "Must specify edge id";
        return QByteArray();
    }
    QUuid edgeId = QUuid(QString(parameters));
    if (edgeId.isNull()) {
        *errorMessage = "Edge id is invalid:" + QString(parameters);
        return QByteArray();
    }
    m_currentDocumentWindow->document()->removeEdge(edgeId);
    return QByteArray();
}

QByteArray RemoteIoConnection::commandRemovePart(const QByteArray &parameters, QString *errorMessage)
{
    COMMAND_PRECHECK();
    
    if (parameters.isEmpty()) {
        *errorMessage = "Must specify part id";
        return QByteArray();
    }
    QUuid partId = QUuid(QString(parameters));
    if (partId.isNull()) {
        *errorMessage = "Part id is invalid:" + QString(parameters);
        return QByteArray();
    }
    m_currentDocumentWindow->document()->removePart(partId);
    return QByteArray();
}

QByteArray RemoteIoConnection::commandAddNode(const QByteArray &parameters, QString *errorMessage)
{
    COMMAND_PRECHECK();
    
    // float x, float y, float z, float radius, QUuid fromNodeId
    
    int offset = 0;
    
    auto xString = nextParameter(parameters, &offset);
    if (xString.isEmpty()) {
        *errorMessage = "Must specify x parameter";
        return QByteArray();
    }
    float x = xString.toFloat();
    
    auto yString = nextParameter(parameters, &offset);
    if (yString.isEmpty()) {
        *errorMessage = "Must specify y parameter";
        return QByteArray();
    }
    float y = yString.toFloat();
    
    auto zString = nextParameter(parameters, &offset);
    if (zString.isEmpty()) {
        *errorMessage = "Must specify z parameter";
        return QByteArray();
    }
    float z = zString.toFloat();
    
    auto radiusString = nextParameter(parameters, &offset);
    if (radiusString.isEmpty()) {
        *errorMessage = "Must specify radius parameter";
        return QByteArray();
    }
    float radius = radiusString.toFloat();
    
    auto fromNodeIdString = nextParameter(parameters, &offset);
    QUuid fromNodeId = QUuid(fromNodeIdString);
    
    m_currentDocumentWindow->document()->addNode(x, y, z, radius, fromNodeId);
    return QByteArray();
}
