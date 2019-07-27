#ifndef DUST3D_SCRIPT_RUNNER_H
#define DUST3D_SCRIPT_RUNNER_H
#include <QObject>
#include <QUuid>
#include <QColor>
#include <QStringList>
#include "snapshot.h"
extern "C" {
#include "quickjs.h"
}

class ScriptRunner : public QObject
{
    Q_OBJECT
public:
    enum class DocumentElementType
    {
        Unknown = 0,
        Canvas,
        Component,
        Part,
        Node
    };

    struct DocumentElement
    {
        QString id = QUuid::createUuid().toString();
        DocumentElementType type = DocumentElementType::Unknown;
        bool deleted = false;
        std::map<QString, QString> attributes = {std::make_pair("id", id)};
    };
    
    struct DocumentComponent : DocumentElement
    {
        DocumentComponent()
        {
            type = DocumentElementType::Component;
        }
        DocumentComponent *parentComponent = nullptr;
    };

    struct DocumentPart : DocumentElement
    {
        DocumentPart()
        {
            type = DocumentElementType::Part;
        }
        DocumentComponent *component = nullptr;
    };
    
    struct DocumentNode : DocumentElement
    {
        DocumentNode()
        {
            type = DocumentElementType::Node;
        }
        DocumentPart *part = nullptr;
    };
    
    struct DocumentCanvas : DocumentElement
    {
        DocumentCanvas()
        {
            type = DocumentElementType::Canvas;
        }
    };
    
    ScriptRunner();
    ~ScriptRunner();
    void run();
    void setScript(QString *script);
    void setVariables(std::map<QString, std::map<QString, QString>> *variables);
    Snapshot *takeResultSnapshot();
    std::map<QString, std::map<QString, QString>> *takeDefaultVariables();
    const QString &scriptError();
    static void mergeVaraibles(std::map<QString, std::map<QString, QString>> *target, const std::map<QString, std::map<QString, QString>> &source);
    DocumentPart *createPart(DocumentComponent *component);
    DocumentComponent *createComponent(DocumentComponent *parentComponent);
    DocumentNode *createNode(DocumentPart *part);
    bool setAttribute(DocumentElement *element, const QString &name, const QString &value);
    QString attribute(DocumentElement *element, const QString &name);
    void connect(DocumentNode *firstNode, DocumentNode *secondNode);
    QString &consoleLog();
    QString createInput(const QString &name, const std::map<QString, QString> &attributes);
    float createFloatInput(const QString &name, float defaultValue, float minValue, float maxValue);
    int createIntInput(const QString &name, int defaultValue, int minValue, int maxValue);
    QColor createColorInput(const QString &name, const QColor &defaultValue);
    bool createCheckInput(const QString &name, bool checked);
    int createSelectInput(const QString &name, int defaultSelectedIndex, const QStringList &options);
signals:
    void finished();
public slots:
    void process();
private:
    QString *m_script = nullptr;
    Snapshot *m_resultSnapshot = nullptr;
    std::map<QString, std::map<QString, QString>> *m_defaultVariables = nullptr;
    std::map<QString, std::map<QString, QString>> *m_variables = nullptr;
    std::vector<DocumentPart *> m_parts;
    std::vector<DocumentComponent *> m_components;
    std::vector<DocumentNode *> m_nodes;
    std::vector<std::pair<DocumentNode *, DocumentNode *>> m_edges;
    QString m_scriptError;
    QString m_consoleLog;
    DocumentCanvas m_canvas;
    void generateSnapshot();
public:
    static JSClassID js_canvasClassId;
    static JSClassID js_partClassId;
    static JSClassID js_componentClassId;
    static JSClassID js_nodeClassId;
};

#endif
