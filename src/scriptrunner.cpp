#include <QDebug>
#include <QElapsedTimer>
#include <QUuid>
#include <QFile>
#include "scriptrunner.h"
#include "util.h"

JSClassID ScriptRunner::js_canvasClassId = 0;
JSClassID ScriptRunner::js_partClassId = 0;
JSClassID ScriptRunner::js_componentClassId = 0;
JSClassID ScriptRunner::js_nodeClassId = 0;

static JSValue js_print(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    
    int i;
    const char *str;

    for (i = 0; i < argc; i++) {
        if (i != 0)
            runner->consoleLog() += ' ';
        str = JS_ToCString(context, argv[i]);
        if (!str)
            return JS_EXCEPTION;
        runner->consoleLog() += str;
        JS_FreeCString(context, str);
    }
    runner->consoleLog() += '\n';
    return JS_UNDEFINED;
}

static ScriptRunner::DocumentElement *GetElementFromArg(JSValueConst arg)
{
    ScriptRunner::DocumentElement *element = nullptr;
    if (nullptr == element) {
        ScriptRunner::DocumentNode *node = (ScriptRunner::DocumentNode *)JS_GetOpaque(arg,
            ScriptRunner::js_nodeClassId);
        if (nullptr != node) {
            element = (ScriptRunner::DocumentElement *)node;
        }
    }
    if (nullptr == element) {
        ScriptRunner::DocumentPart *part = (ScriptRunner::DocumentPart *)JS_GetOpaque(arg,
            ScriptRunner::js_partClassId);
        if (nullptr != part) {
            element = (ScriptRunner::DocumentElement *)part;
        }
    }
    if (nullptr == element) {
        ScriptRunner::DocumentComponent *component = (ScriptRunner::DocumentComponent *)JS_GetOpaque(arg,
            ScriptRunner::js_componentClassId);
        if (nullptr != component) {
            element = (ScriptRunner::DocumentElement *)component;
        }
    }
    if (nullptr == element) {
        ScriptRunner::DocumentCanvas *canvas = (ScriptRunner::DocumentCanvas *)JS_GetOpaque(arg,
            ScriptRunner::js_canvasClassId);
        if (nullptr != canvas) {
            element = (ScriptRunner::DocumentElement *)canvas;
        }
    }
    return element;
}

static JSValue js_setAttribute(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 3) {
        runner->consoleLog() += "Incomplete parameters, expect: element, attributeName, attributeValue\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *element = GetElementFromArg(argv[0]);
    if (nullptr == element) {
        runner->consoleLog() += "Parameters error\r\n";
        return JS_EXCEPTION;
    }
    const char *attributeName = nullptr;
    const char *attributeValue = nullptr;
    attributeName = JS_ToCString(context, argv[1]);
    if (!attributeName)
        goto fail;
    attributeValue = JS_ToCString(context, argv[2]);
    if (!attributeValue)
        goto fail;
    runner->setAttribute(element, attributeName, attributeValue);
    JS_FreeCString(context, attributeName);
    JS_FreeCString(context, attributeValue);
    return JS_UNDEFINED;
fail:
    JS_FreeCString(context, attributeName);
    JS_FreeCString(context, attributeValue);
    return JS_EXCEPTION;
}

static JSValue js_attribute(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 2) {
        runner->consoleLog() += "Incomplete parameters, expect: element, attributeName\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *element = GetElementFromArg(argv[0]);
    if (nullptr == element) {
        runner->consoleLog() += "Parameters error\r\n";
        return JS_EXCEPTION;
    }
    QString attributeValue;
    const char *attributeName = nullptr;
    attributeName = JS_ToCString(context, argv[1]);
    if (!attributeName)
        goto fail;
    attributeValue = runner->attribute(element, attributeName);
    JS_FreeCString(context, attributeName);
    return JS_NewString(context, attributeValue.toUtf8().constData());
fail:
    JS_FreeCString(context, attributeName);
    return JS_EXCEPTION;
}

static JSValue js_connect(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 2) {
        runner->consoleLog() += "Incomplete parameters, expect: firstNode, secondNode\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *firstElement = (ScriptRunner::DocumentElement *)JS_GetOpaque(argv[0],
        ScriptRunner::js_nodeClassId);
    if (nullptr == firstElement ||
            ScriptRunner::DocumentElementType::Node != firstElement->type) {
        runner->consoleLog() += "First parameter must be node\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *secondElement = (ScriptRunner::DocumentElement *)JS_GetOpaque(argv[1],
        ScriptRunner::js_nodeClassId);
    if (nullptr == secondElement ||
            ScriptRunner::DocumentElementType::Node != secondElement->type) {
        runner->consoleLog() += "Second parameter must be node\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentNode *firstNode = (ScriptRunner::DocumentNode *)firstElement;
    ScriptRunner::DocumentNode *secondNode = (ScriptRunner::DocumentNode *)secondElement;
    if (firstNode->part != secondNode->part) {
        runner->consoleLog() += "Cannot connect nodes come from different parts\r\n";
        return JS_EXCEPTION;
    }
    runner->connect(firstNode, secondNode);
    return JS_UNDEFINED;
}

static JSValue js_createComponent(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    ScriptRunner::DocumentComponent *parentComponent = nullptr;
    if (argc >= 1) {
        ScriptRunner::DocumentElement *element = (ScriptRunner::DocumentElement *)JS_GetOpaque(argv[0],
            ScriptRunner::js_componentClassId);
        if (nullptr == element ||
                ScriptRunner::DocumentElementType::Component != element->type) {
            runner->consoleLog() += "First parameter must be null or component\r\n";
            return JS_EXCEPTION;
        }
        parentComponent = (ScriptRunner::DocumentComponent *)element;
    }
    JSValue component = JS_NewObjectClass(context, ScriptRunner::js_componentClassId);
    JS_SetOpaque(component, runner->createComponent(parentComponent));
    return component;
}

static JSValue js_createPart(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 1) {
        runner->consoleLog() += "Incomplete parameters, expect: component\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *element = (ScriptRunner::DocumentElement *)JS_GetOpaque(argv[0],
        ScriptRunner::js_componentClassId);
    if (nullptr == element ||
            ScriptRunner::DocumentElementType::Component != element->type) {
        runner->consoleLog() += "First parameter must be component\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentComponent *component = (ScriptRunner::DocumentComponent *)element;
    JSValue part = JS_NewObjectClass(context, ScriptRunner::js_partClassId);
    JS_SetOpaque(part, runner->createPart(component));
    return part;
}

static JSValue js_createNode(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 1) {
        runner->consoleLog() += "Incomplete parameters, expect: part\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentElement *element = (ScriptRunner::DocumentElement *)JS_GetOpaque(argv[0],
        ScriptRunner::js_partClassId);
    if (nullptr == element ||
            ScriptRunner::DocumentElementType::Part != element->type) {
        runner->consoleLog() += "First parameter must be part\r\n";
        return JS_EXCEPTION;
    }
    ScriptRunner::DocumentPart *part = (ScriptRunner::DocumentPart *)element;
    JSValue node = JS_NewObjectClass(context, ScriptRunner::js_nodeClassId);
    JS_SetOpaque(node, runner->createNode(part));
    return node;
}

static JSValue js_createFloatInput(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 4) {
        runner->consoleLog() += "Incomplete parameters, expect: name, defaultValue, minValue, maxValue\r\n";
        return JS_EXCEPTION;
    }
    
    double mergedValue = 0.0;
    
    const char *name = nullptr;
    double defaultValue = 0.0;
    double minValue = 0.0;
    double maxValue = 0.0;
    
    name = JS_ToCString(context, argv[0]);
    if (!name)
        goto fail;
    JS_ToFloat64(context, &defaultValue, argv[1]);
    JS_ToFloat64(context, &minValue, argv[2]);
    JS_ToFloat64(context, &maxValue, argv[3]);
    
    mergedValue = runner->createFloatInput(name, defaultValue, minValue, maxValue);
    JS_FreeCString(context, name);
    
    return JS_NewFloat64(context, mergedValue);
    
fail:
    JS_FreeCString(context, name);
    return JS_EXCEPTION;
}

static JSValue js_createIntInput(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 4) {
        runner->consoleLog() += "Incomplete parameters, expect: name, defaultValue, minValue, maxValue\r\n";
        return JS_EXCEPTION;
    }
    
    int64_t mergedValue = 0.0;
    
    const char *name = nullptr;
    int64_t defaultValue = 0;
    int64_t minValue = 0;
    int64_t maxValue = 0;
    
    name = JS_ToCString(context, argv[0]);
    if (!name)
        goto fail;
    JS_ToInt64(context, &defaultValue, argv[1]);
    JS_ToInt64(context, &minValue, argv[2]);
    JS_ToInt64(context, &maxValue, argv[3]);
    
    mergedValue = runner->createIntInput(name, defaultValue, minValue, maxValue);
    JS_FreeCString(context, name);
    
    return JS_NewInt64(context, mergedValue);
    
fail:
    JS_FreeCString(context, name);
    return JS_EXCEPTION;
}

static JSValue js_createColorInput(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 2) {
        runner->consoleLog() += "Incomplete parameters, expect: name, defaultValue\r\n";
        return JS_EXCEPTION;
    }
    
    QColor mergeValue;
    
    const char *name = nullptr;
    const char *defaultValue = nullptr;
    
    name = JS_ToCString(context, argv[0]);
    if (!name)
        goto fail;
    defaultValue = JS_ToCString(context, argv[1]);
    if (!defaultValue)
        goto fail;
    
    mergeValue = runner->createColorInput(name, defaultValue);
    
    JS_FreeCString(context, name);
    JS_FreeCString(context, defaultValue);
    
    return JS_NewString(context, mergeValue.name().toUtf8().constData());
    
fail:
    JS_FreeCString(context, name);
    JS_FreeCString(context, defaultValue);
    return JS_EXCEPTION;
}

static JSValue js_createCheckInput(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 2) {
        runner->consoleLog() += "Incomplete parameters, expect: name, defaultValue\r\n";
        return JS_EXCEPTION;
    }
    
    bool mergedValue = false;
    
    const char *name = nullptr;
    const char *value = nullptr;
    bool defaultValue = false;
    
    name = JS_ToCString(context, argv[0]);
    if (!name)
        goto fail;
    value = JS_ToCString(context, argv[1]);
    if (!value)
        goto fail;
    
    defaultValue = isTrueValueString(value);
    mergedValue = runner->createCheckInput(name, defaultValue);
    JS_FreeCString(context, name);
    JS_FreeCString(context, value);
    
    return JS_NewBool(context, mergedValue);
    
fail:
    JS_FreeCString(context, name);
    JS_FreeCString(context, value);
    return JS_EXCEPTION;
}

static JSValue js_createSelectInput(JSContext *context, JSValueConst thisValue,
    int argc, JSValueConst *argv)
{
    ScriptRunner *runner = (ScriptRunner *)JS_GetContextOpaque(context);
    if (argc < 3) {
        runner->consoleLog() += "Incomplete parameters, expect: name, defaultValue, options\r\n";
        return JS_EXCEPTION;
    }
    
    const char *name = nullptr;
    int32_t defaultValue = 0;
    int32_t mergedValue = 0;
    JSValue *arrayItems = nullptr;
    uint32_t arrayLength = 0;
    QStringList options;
    
    name = JS_ToCString(context, argv[0]);
    if (!name)
        goto fail;
    
    JS_ToInt32(context, &defaultValue, argv[1]);
    
    if (true != JS_IsArray(context, argv[2])) {
        runner->consoleLog() += "Expect array as the third parameter\r\n";
        goto fail;
    }
    
    if (!js_get_fast_array(context, argv[2], &arrayItems, &arrayLength)) {
        runner->consoleLog() += "Read third parameter as array failed\r\n";
        goto fail;
    }
    
    for (uint32_t i = 0; i < arrayLength; ++i) {
        const char *optionValue = JS_ToCString(context, arrayItems[i]);
        if (nullptr != optionValue) {
            options.append(optionValue);
            JS_FreeCString(context, optionValue);
        } else {
            options.append("");
        }
    }
    
    mergedValue = runner->createSelectInput(name, defaultValue, options);
    if (mergedValue < 0)
        mergedValue = 0;
    else if (mergedValue >= options.size())
        mergedValue = options.size() - 1;
    
    JS_FreeCString(context, name);

    return JS_NewInt32(context, mergedValue);
    
fail:
    JS_FreeCString(context, name);
    return JS_EXCEPTION;
}

ScriptRunner::ScriptRunner()
{
}

ScriptRunner::~ScriptRunner()
{
    delete m_resultSnapshot;
    delete m_defaultVariables;
    for (auto &it: m_parts)
        delete it;
    for (auto &it: m_components)
        delete it;
    for (auto &it: m_nodes)
        delete it;
}

QString &ScriptRunner::consoleLog()
{
    return m_consoleLog;
}

const QString &ScriptRunner::scriptError()
{
    return m_scriptError;
}

ScriptRunner::DocumentPart *ScriptRunner::createPart(DocumentComponent *component)
{
    ScriptRunner::DocumentPart *part = new ScriptRunner::DocumentPart;
    part->component = component;
    m_parts.push_back(part);
    return part;
}

ScriptRunner::DocumentComponent *ScriptRunner::createComponent(DocumentComponent *parentComponent)
{
    ScriptRunner::DocumentComponent *component = new ScriptRunner::DocumentComponent;
    component->parentComponent = parentComponent;
    m_components.push_back(component);
    return component;
}

ScriptRunner::DocumentNode *ScriptRunner::createNode(DocumentPart *part)
{
    ScriptRunner::DocumentNode *node = new ScriptRunner::DocumentNode;
    node->part = part;
    m_nodes.push_back(node);
    return node;
}

bool ScriptRunner::setAttribute(DocumentElement *element, const QString &name, const QString &value)
{
    element->attributes[name] = value;
    
    // TODO: Validate attribute name and value
    
    return true;
}

QString ScriptRunner::attribute(DocumentElement *element, const QString &name)
{
    auto findAttribute = element->attributes.find(name);
    if (findAttribute == element->attributes.end())
        return QString();
    return findAttribute->second;
}

void ScriptRunner::connect(DocumentNode *firstNode, DocumentNode *secondNode)
{
    m_edges.push_back(std::make_pair(firstNode, secondNode));
}

static void js_componentFinalizer(JSRuntime *runtime, JSValue value)
{
    ScriptRunner::DocumentComponent *component = (ScriptRunner::DocumentComponent *)JS_GetOpaque(value,
        ScriptRunner::js_componentClassId);
    if (component) {
        component->deleted = true;
    }
}

static JSClassDef js_componentClass = {
    "Component",
    js_componentFinalizer,
};

static void js_nodeFinalizer(JSRuntime *runtime, JSValue value)
{
    ScriptRunner::DocumentNode *node = (ScriptRunner::DocumentNode *)JS_GetOpaque(value,
        ScriptRunner::js_nodeClassId);
    if (node) {
        node->deleted = true;
    }
}

static JSClassDef js_nodeClass = {
    "Node",
    js_nodeFinalizer,
};

static void js_partFinalizer(JSRuntime *runtime, JSValue value)
{
    ScriptRunner::DocumentPart *part = (ScriptRunner::DocumentPart *)JS_GetOpaque(value,
        ScriptRunner::js_partClassId);
    if (part) {
        part->deleted = true;
    }
}

static JSClassDef js_partClass = {
    "Part",
    js_partFinalizer,
};

void ScriptRunner::run()
{
    QElapsedTimer countTimeConsumed;
    countTimeConsumed.start();
    
    // Warning: Not thread safe, but we have only one script instance running, so it doesn't matter
    js_canvasClassId = JS_NewClassID(&js_canvasClassId);
    js_partClassId = JS_NewClassID(&js_partClassId);
    js_componentClassId = JS_NewClassID(&js_componentClassId);
    js_nodeClassId = JS_NewClassID(&js_nodeClassId);
    
    m_defaultVariables = new std::map<QString, std::map<QString, QString>>;

    JSRuntime *runtime = JS_NewRuntime();
    JSContext *context = JS_NewContext(runtime);
    
    JS_NewClass(runtime, js_partClassId, &js_partClass);
    JS_NewClass(runtime, js_componentClassId, &js_componentClass);
    JS_NewClass(runtime, js_nodeClassId, &js_nodeClass);
    
    JS_SetContextOpaque(context, this);
    
    if (nullptr != m_script &&
            !m_script->trimmed().isEmpty()) {
        auto buffer = m_script->toUtf8();
        
        JSValue globalObject = JS_GetGlobalObject(context);
        
        JSValue document = JS_NewObject(context);
        
            JS_SetPropertyStr(context,
                document, "createComponent",
                JS_NewCFunction(context, js_createComponent, "createComponent", 1));
            JS_SetPropertyStr(context,
                document, "createPart",
                JS_NewCFunction(context, js_createPart, "createPart", 1));
            JS_SetPropertyStr(context,
                document, "createNode",
                JS_NewCFunction(context, js_createNode, "createNode", 1));
            JS_SetPropertyStr(context,
                document, "createFloatInput",
                JS_NewCFunction(context, js_createFloatInput, "createFloatInput", 4));
            JS_SetPropertyStr(context,
                document, "createIntInput",
                JS_NewCFunction(context, js_createIntInput, "createIntInput", 4));
            JS_SetPropertyStr(context,
                document, "createColorInput",
                JS_NewCFunction(context, js_createColorInput, "createColorInput", 2));
            JS_SetPropertyStr(context,
                document, "createCheckInput",
                JS_NewCFunction(context, js_createCheckInput, "createCheckInput", 2));
            JS_SetPropertyStr(context,
                document, "createSelectInput",
                JS_NewCFunction(context, js_createSelectInput, "createSelectInput", 3));
            JS_SetPropertyStr(context,
                document, "connect",
                JS_NewCFunction(context, js_connect, "connect", 2));
            JS_SetPropertyStr(context,
                document, "setAttribute",
                JS_NewCFunction(context, js_setAttribute, "setAttribute", 3));
            JS_SetPropertyStr(context,
                document, "attribute",
                JS_NewCFunction(context, js_attribute, "attribute", 2));
        
            JSValue canvas = JS_NewObjectClass(context, ScriptRunner::js_canvasClassId);
            JS_SetOpaque(canvas, &m_canvas);
            JS_SetPropertyStr(context,
                document, "canvas",
                canvas);
        
        JS_SetPropertyStr(context, globalObject, "document", document);
        
        JSValue console = JS_NewObject(context);
        JS_SetPropertyStr(context,
            console, "log",
            JS_NewCFunction(context, js_print, "log", 1));
        JS_SetPropertyStr(context,
            console, "warn",
            JS_NewCFunction(context, js_print, "warn", 1));
        JS_SetPropertyStr(context,
            console, "error",
            JS_NewCFunction(context, js_print, "error", 1));
        JS_SetPropertyStr(context, globalObject, "console", console);
        
        QFile file(":/thirdparty/three.js/dust3d.three.js");
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileContent = file.readAll();
            JSValue three = JS_NewObject(context);
            JS_SetPropertyStr(context, globalObject, "THREE", three);
            JSValue object = JS_Eval(context, (char *)fileContent.constData(), fileContent.size(), "<thirdparty/three.js/dust3d.three.js>",
                  JS_EVAL_TYPE_GLOBAL);
            JS_FreeValue(context, object);
            file.close();
        }
        
        JS_FreeValue(context, globalObject);
        
        JSValue object = JS_Eval(context, buffer.constData(), buffer.size(), "<input>",
            JS_EVAL_TYPE_GLOBAL);
        if (JS_IsException(object)) {
            JSValue exceptionValue = JS_GetException(context);
            bool isError = JS_IsError(context, exceptionValue);
            if (isError) {
                m_scriptError += "Throw: ";
                const char *exceptionError = JS_ToCString(context, exceptionValue);
                m_scriptError += exceptionError;
                m_scriptError += "\r\n";
                JS_FreeCString(context, exceptionError);
                JSValue value = JS_GetPropertyStr(context, exceptionValue, "stack");
                if (!JS_IsUndefined(value)) {
                    const char *stack = JS_ToCString(context, value);
                    m_scriptError += stack;
                    m_scriptError += "\r\n";
                    JS_FreeCString(context, stack);
                }
                JS_FreeValue(context, value);
                
                qDebug() << "QuickJS" << m_scriptError;
            }
        } else {
            generateSnapshot();
            const char *objectString = JS_ToCString(context, object);
            qDebug() << "Result:" << objectString;
            JS_FreeCString(context, objectString);
        }
        
        JS_FreeValue(context, object);
    }
    
    JS_FreeContext(context);
    JS_FreeRuntime(runtime);
    
    qDebug() << "The script run" << countTimeConsumed.elapsed() << "milliseconds";
}

void ScriptRunner::generateSnapshot()
{
    m_resultSnapshot = new Snapshot;
    std::map<void *, QString> pointerToIdMap;
    
    std::map<void *, QStringList> componentChildrensMap;
    
    QStringList rootChildren;
    
    m_resultSnapshot->canvas = m_canvas.attributes;
    
    for (const auto &it: m_components) {
        QString idString = it->id;
        pointerToIdMap[it] = idString;
        auto &component = m_resultSnapshot->components[idString];
        component = it->attributes;
        component["id"] = idString;
        componentChildrensMap[it->parentComponent].append(idString);
        if (nullptr == it->parentComponent)
            rootChildren.append(idString);
    }
    m_resultSnapshot->rootComponent["children"] = rootChildren.join(",");
    
    for (const auto &it: m_components) {
        const auto &idString = pointerToIdMap[it];
        auto &component = m_resultSnapshot->components[idString];
        auto &childrens = componentChildrensMap[it];
        if (!childrens.empty())
            component["children"] = childrens.join(",");
    }
    
    for (const auto &it: m_parts) {
        auto findComponent = pointerToIdMap.find(it->component);
        if (findComponent == pointerToIdMap.end()) {
            m_scriptError += "Find component pointer failed, component maybe deleted\r\n";
            continue;
        }
        
        QString idString = it->id;
        pointerToIdMap[it] = idString;
        auto &part = m_resultSnapshot->parts[idString];
        part = it->attributes;
        part.insert({"visible", "true"});
        part["id"] = idString;
        
        auto &component = m_resultSnapshot->components[findComponent->second];
        component["linkData"] = idString;
        component["linkDataType"] = "partId";
    }
    for (const auto &it: m_nodes) {
        auto findPart = pointerToIdMap.find(it->part);
        if (findPart == pointerToIdMap.end()) {
            m_scriptError += "Find part pointer failed, part maybe deleted\r\n";
            continue;
        }
        QString idString = it->id;
        pointerToIdMap[it] = idString;
        auto &node = m_resultSnapshot->nodes[idString];
        node = it->attributes;
        node["id"] = idString;
        node["partId"] = findPart->second;
    }
    for (const auto &it: m_edges) {
        if (it.first->part != it.second->part) {
            m_scriptError += "Cannot connect nodes come from different parts\r\n";
            continue;
        }
        auto findFirstNode = pointerToIdMap.find(it.first);
        if (findFirstNode == pointerToIdMap.end()) {
            m_scriptError += "Find first node pointer failed, node maybe deleted\r\n";
            continue;
        }
        auto findSecondNode = pointerToIdMap.find(it.second);
        if (findSecondNode == pointerToIdMap.end()) {
            m_scriptError += "Find second node pointer failed, node maybe deleted\r\n";
            continue;
        }
        auto findPart = pointerToIdMap.find(it.first->part);
        if (findPart == pointerToIdMap.end()) {
            m_scriptError += "Find part pointer failed, part maybe deleted\r\n";
            continue;
        }
        QString idString = QUuid::createUuid().toString();
        auto &edge = m_resultSnapshot->edges[idString];
        edge["id"] = idString;
        edge["from"] = findFirstNode->second;
        edge["to"] = findSecondNode->second;
        edge["partId"] = findPart->second;
    }
    
    for (const auto &it: m_resultSnapshot->nodes) {
        qDebug() << "Generated node:" << it.second;
    }
    for (const auto &it: m_resultSnapshot->edges) {
        qDebug() << "Generated edge:" << it.second;
    }
    for (const auto &it: m_resultSnapshot->parts) {
        qDebug() << "Generated part:" << it.second;
    }
    for (const auto &it: m_resultSnapshot->components) {
        qDebug() << "Generated component:" << it.second;
    }
}

QString ScriptRunner::createInput(const QString &name, const std::map<QString, QString> &attributes)
{
    if (nullptr != m_defaultVariables) {
        if (m_defaultVariables->find(name) != m_defaultVariables->end()) {
            m_scriptError += "Repeated variable name found: \"" + name + "\"\r\n";
        }
        (*m_defaultVariables)[name] = attributes;
    }
    if (nullptr != m_variables) {
        auto findVariable = m_variables->find(name);
        if (findVariable != m_variables->end()) {
            auto findValue = findVariable->second.find("value");
            if (findValue != findVariable->second.end())
                return findValue->second;
        }
    }
    auto findValue = attributes.find("value");
    if (findValue != attributes.end())
        return findValue->second;
    return QString();
}

float ScriptRunner::createFloatInput(const QString &name, float defaultValue, float minValue, float maxValue)
{
    std::map<QString, QString> attributes;
    attributes["input"] = "Float";
    attributes["value"] = QString::number(defaultValue);
    attributes["defaultValue"] = attributes["value"];
    attributes["minValue"] = QString::number(minValue);
    attributes["maxValue"] = QString::number(maxValue);
    auto inputValue = createInput(name, attributes);
    return inputValue.toFloat();
}

int ScriptRunner::createIntInput(const QString &name, int defaultValue, int minValue, int maxValue)
{
    std::map<QString, QString> attributes;
    attributes["input"] = "Int";
    attributes["value"] = QString::number(defaultValue);
    attributes["defaultValue"] = attributes["value"];
    attributes["minValue"] = QString::number(minValue);
    attributes["maxValue"] = QString::number(maxValue);
    auto inputValue = createInput(name, attributes);
    return inputValue.toInt();
}

QColor ScriptRunner::createColorInput(const QString &name, const QColor &defaultValue)
{
    std::map<QString, QString> attributes;
    attributes["input"] = "Color";
    attributes["value"] = defaultValue.name();
    attributes["defaultValue"] = attributes["value"];
    auto inputValue = createInput(name, attributes);
    return QColor(inputValue);
}

bool ScriptRunner::createCheckInput(const QString &name, bool checked)
{
    std::map<QString, QString> attributes;
    attributes["input"] = "Check";
    attributes["value"] = checked ? "true" : "false";
    attributes["defaultValue"] = attributes["value"];
    auto inputValue = createInput(name, attributes);
    return isTrueValueString(inputValue);
}

int ScriptRunner::createSelectInput(const QString &name, int defaultSelectedIndex, const QStringList &options)
{
    std::map<QString, QString> attributes;
    attributes["input"] = "Select";
    attributes["value"] = QString::number(defaultSelectedIndex);
    attributes["defaultValue"] = attributes["value"];
    attributes["length"] = QString::number(options.size());
    for (int i = 0; i < options.size(); ++i) {
        attributes["option" + QString::number(i)] = options[i];
    }
    auto inputValue = createInput(name, attributes);
    int selectedIndex = inputValue.toInt();
    if (selectedIndex >= options.size()) {
        m_scriptError += QString("Selected index of select input \"%1\" is been reset to 0 because of out of range\r\n").arg(name);
        selectedIndex = 0;
    }
    return selectedIndex;
}

void ScriptRunner::process()
{
    run();
    emit finished();
}

void ScriptRunner::setScript(QString *script)
{
    m_script = script;
}

void ScriptRunner::setVariables(std::map<QString, std::map<QString, QString>> *variables)
{
    m_variables = variables;
}

Snapshot *ScriptRunner::takeResultSnapshot()
{
    Snapshot *snapshot = m_resultSnapshot;
    m_resultSnapshot = nullptr;
    return snapshot;
}

std::map<QString, std::map<QString, QString>> *ScriptRunner::takeDefaultVariables()
{
    std::map<QString, std::map<QString, QString>> *defaultVariables = m_defaultVariables;
    m_defaultVariables = nullptr;
    return defaultVariables;
}

void ScriptRunner::mergeVaraibles(std::map<QString, std::map<QString, QString>> *target, const std::map<QString, std::map<QString, QString>> &source)
{
    for (const auto &it: source) {
        (*target)[it.first] = it.second;
    }
}
