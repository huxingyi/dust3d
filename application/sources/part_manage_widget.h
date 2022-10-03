#ifndef DUST3D_APPLICATION_PART_MANAGE_WIDGET_H_
#define DUST3D_APPLICATION_PART_MANAGE_WIDGET_H_

#include <QWidget>

class Document;

class PartManageWidget : public QWidget
{
    Q_OBJECT
public:
    PartManageWidget(const Document *document, QWidget *parent=nullptr);
};

#endif
