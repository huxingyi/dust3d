#ifndef SKELETON_DOCUMENT_WINDOW_H
#define SKELETON_DOCUMENT_WINDOW_H
#include <QMainWindow>
#include <QShowEvent>
#include <QPushButton>
#include "skeletondocument.h"
#include "modelwidget.h"

class SkeletonDocumentWindow : public QMainWindow
{
    Q_OBJECT
signals:
    void initialized();
public:
    SkeletonDocumentWindow();
    ~SkeletonDocumentWindow();
protected:
    void showEvent(QShowEvent *event);
public slots:
    void changeTurnaround();
private:
    void initButton(QPushButton *button);
private:
    SkeletonDocument *m_document;
    bool m_firstShow;
    ModelWidget *m_modelWidget;
};

#endif

