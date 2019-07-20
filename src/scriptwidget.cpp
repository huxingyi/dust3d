#include <QVBoxLayout>
#include <QSplitter>
#include "scriptwidget.h"
#include "scripteditwidget.h"
#include "theme.h"

ScriptWidget::ScriptWidget(const Document *document, QWidget *parent) :
    QWidget(parent),
    m_document(document)
{
    m_consoleEdit = new QPlainTextEdit;
    m_consoleEdit->setReadOnly(true);
    m_consoleEdit->hide();
    
    ScriptVariablesWidget *scriptVariablesWidget = new ScriptVariablesWidget(m_document);
    scriptVariablesWidget->hide();
    connect(m_document, &Document::mergedVaraiblesChanged, this, [=]() {
        if (m_document->variables().empty())
            scriptVariablesWidget->hide();
        else
            scriptVariablesWidget->show();
    });
    
    ScriptEditWidget *scriptEditWidget = new ScriptEditWidget;

    connect(m_document, &Document::cleanupScript, scriptEditWidget, &ScriptEditWidget::clear);
    connect(m_document, &Document::scriptModifiedFromExternal, this, [=]() {
        scriptEditWidget->setPlainText(document->script());
    });
    connect(m_document, &Document::scriptErrorChanged, this, &ScriptWidget::updateScriptConsole);
    connect(m_document, &Document::scriptConsoleLogChanged, this, &ScriptWidget::updateScriptConsole);

    connect(scriptEditWidget, &ScriptEditWidget::scriptChanged, m_document, &Document::updateScript);
    
    connect(m_document, &Document::mergedVaraiblesChanged, scriptVariablesWidget, &ScriptVariablesWidget::reload);
    
    QSplitter *splitter = new QSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(scriptEditWidget);
    splitter->addWidget(m_consoleEdit);
    splitter->addWidget(scriptVariablesWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(splitter);
    setLayout(mainLayout);
}

QSize ScriptWidget::sizeHint() const
{
    return QSize(Theme::sidebarPreferredWidth, 0);
}

void ScriptWidget::updateScriptConsole()
{
    const auto &scriptError = m_document->scriptError();
    const auto &scriptConsoleLog = m_document->scriptConsoleLog();
    if (scriptError.isEmpty() && scriptConsoleLog.isEmpty()) {
        m_consoleEdit->hide();
    } else {
        m_consoleEdit->setPlainText(scriptError + scriptConsoleLog);
        m_consoleEdit->show();
    }
}

