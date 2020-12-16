#ifndef DUST3D_BONE_DOCUMENT_H
#define DUST3D_BONE_DOCUMENT_H
#include "skeletondocument.h"

class BoneDocument : public SkeletonDocument
{
    Q_OBJECT
signals:
    void turnaroundChanged();
public:
    BoneDocument();
    bool undoable(void) const;
    bool redoable(void) const;
    bool hasPastableNodesInClipboard(void) const;
    bool originSettled(void) const;
    bool isNodeEditable(QUuid) const;
    bool isEdgeEditable(QUuid) const;
    void copyNodes(std::set<QUuid> nodeIdSet) const;
    void undo(void);
    void redo(void);
    void paste(void);
    void updateTurnaround(const QImage &image);
};

#endif
