#ifndef SCRIPT_WIDGET_H
#define SCRIPT_WIDGET_H
#include <QWidget>
#include <QPlainTextEdit>
#include "document.h"
#include "scriptvariableswidget.h"

class ScriptWidget : public QWidget
{
    Q_OBJECT
public:
    ScriptWidget(const Document *document, QWidget *parent=nullptr);
public slots:
    void updateScriptConsole();
protected:
    QSize sizeHint() const override;
private:
    const Document *m_document = nullptr;
    QPlainTextEdit *m_consoleEdit = nullptr;
};

#endif
