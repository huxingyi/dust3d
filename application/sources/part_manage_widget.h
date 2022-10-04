#ifndef DUST3D_APPLICATION_PART_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_PART_MANAGE_WIDGET_H_

#include <QWidget>

class Document;
class ComponentPreviewGridWidget;
class QPushButton;

class PartManageWidget : public QWidget
{
    Q_OBJECT
public:
    PartManageWidget(const Document *document, QWidget *parent=nullptr);
private:
    const Document *m_document = nullptr;
    ComponentPreviewGridWidget *m_componentPreviewGridWidget = nullptr;
    QPushButton *m_levelUpButton = nullptr;
    QPushButton *m_lockButton = nullptr;
    QPushButton *m_unlockButton = nullptr;
    QPushButton *m_showButton = nullptr;
    QPushButton *m_hideButton = nullptr;
    QPushButton *m_unlinkButton = nullptr;
    QPushButton *m_linkButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    void updateToolButtons();
    void updateLevelUpButton();
};

#endif
