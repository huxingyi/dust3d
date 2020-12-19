#include "bonedocument.h"

BoneDocument::BoneDocument()
{
}

bool BoneDocument::undoable(void) const
{
    return false;
}

bool BoneDocument::redoable(void) const
{
    return false;
}

bool BoneDocument::hasPastableNodesInClipboard(void) const
{
    return false;
}

bool BoneDocument::originSettled(void) const
{
    return false;
}

bool BoneDocument::isNodeEditable(QUuid) const
{
    return false;
}

bool BoneDocument::isEdgeEditable(QUuid) const
{
    return false;
}

void BoneDocument::copyNodes(std::set<QUuid> nodeIdSet) const
{
    // TODO:
}


void BoneDocument::undo(void)
{
    // TODO:
}

void BoneDocument::redo(void)
{
    // TODO:
}

void BoneDocument::paste(void)
{
    // TODO:
}

void BoneDocument::updateTurnaround(const QImage &image)
{
    turnaround = image;
    emit turnaroundChanged();
}

void BoneDocument::setEditMode(SkeletonDocumentEditMode mode)
{
    if (editMode == mode)
        return;

    editMode = mode;
    emit editModeChanged();
}

void BoneDocument::setXlockState(bool locked)
{
    if (xlocked == locked)
        return;
    xlocked = locked;
    emit xlockStateChanged();
}

void BoneDocument::setYlockState(bool locked)
{
    if (ylocked == locked)
        return;
    ylocked = locked;
    emit ylockStateChanged();
}

void BoneDocument::setZlockState(bool locked)
{
    if (zlocked == locked)
        return;
    zlocked = locked;
    emit zlockStateChanged();
}

void BoneDocument::setRadiusLockState(bool locked)
{
    if (radiusLocked == locked)
        return;
    radiusLocked = locked;
    emit radiusLockStateChanged();
}
