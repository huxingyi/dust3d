#ifndef DUST3D_CUT_DOCUMENT_H
#define DUST3D_CUT_DOCUMENT_H
#include <deque>
#include <QVector2D>
#include "skeletondocument.h"

struct CutHistoryItem
{
    std::vector<QVector2D> cutTemplate;
};

class CutDocument : public SkeletonDocument
{
    Q_OBJECT
signals:
    void cleanup();
    void nodeAdded(QUuid nodeId);
    void edgeAdded(QUuid edgeId);
    void nodeOriginChanged(QUuid nodeId);
    void cutTemplateChanged();
    
public:
    bool undoable() const override;
    bool redoable() const override;
    bool hasPastableNodesInClipboard() const override;
    bool originSettled() const override;
    bool isNodeEditable(QUuid nodeId) const override;
    bool isEdgeEditable(QUuid edgeId) const override;
    void copyNodes(std::set<QUuid> nodeIdSet) const override;
    
    void reset();
    void toCutTemplate(std::vector<QVector2D> &cutTemplate);
    void fromCutTemplate(const std::vector<QVector2D> &cutTemplate);
    
public slots:
    void saveHistoryItem();
    void clearHistories();
    void undo() override;
    void redo() override;
    void paste() override;
    
    void moveNodeBy(QUuid nodeId, float x, float y, float z);
    void setNodeOrigin(QUuid nodeId, float x, float y, float z);
    
public:
    static const float m_nodeRadius;
    static const float m_nodeScaleFactor;
    
private:
    std::deque<CutHistoryItem> m_undoItems;
    std::deque<CutHistoryItem> m_redoItems;
    std::vector<QUuid> m_cutNodeIds;
    QUuid m_partId;
    
    QVector2D nodeToCutPosition(const QVector3D &nodePosition);
    QVector3D cutToNodePosition(const QVector2D &cutPosition);
};

#endif

