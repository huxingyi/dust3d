#include <QShortcut>
#include <QObject>
#include "shortcuts.h"

#define defineKey(keyVal, funcName) do {                                     \
    auto key = new QShortcut(widget);                                        \
    key->setKey(keyVal);                                                     \
    QObject::connect(key, &QShortcut::activated,                             \
        graphicsWidget, funcName);                                           \
} while (false)

void initShortCuts(QWidget *widget, SkeletonGraphicsWidget *graphicsWidget)
{
    defineKey(Qt::Key_Escape, &SkeletonGraphicsWidget::shortcutEscape);
    defineKey(Qt::Key_Delete, &SkeletonGraphicsWidget::shortcutDelete);
    defineKey(Qt::Key_Backspace, &SkeletonGraphicsWidget::shortcutDelete);
    defineKey(Qt::Key_A, &SkeletonGraphicsWidget::shortcutAddMode);
    //defineKey(Qt::Key_G, &SkeletonGraphicsWidget::shortcutMarkMode);
    defineKey(Qt::CTRL + Qt::Key_A, &SkeletonGraphicsWidget::shortcutSelectAll);
    defineKey(Qt::CTRL + Qt::Key_Z, &SkeletonGraphicsWidget::shortcutUndo);
    defineKey(Qt::CTRL + Qt::SHIFT + Qt::Key_Z, &SkeletonGraphicsWidget::shortcutRedo);
    defineKey(Qt::CTRL + Qt::Key_Y, &SkeletonGraphicsWidget::shortcutRedo);
    defineKey(Qt::Key_Z, &SkeletonGraphicsWidget::shortcutZlock);
    defineKey(Qt::Key_Y, &SkeletonGraphicsWidget::shortcutYlock);
    defineKey(Qt::CTRL + Qt::Key_X, &SkeletonGraphicsWidget::shortcutCut);
    defineKey(Qt::Key_X, &SkeletonGraphicsWidget::shortcutXlock);
    defineKey(Qt::CTRL + Qt::Key_C, &SkeletonGraphicsWidget::shortcutCopy);
    defineKey(Qt::CTRL + Qt::Key_V, &SkeletonGraphicsWidget::shortcutPaste);
    defineKey(Qt::Key_S, &SkeletonGraphicsWidget::shortcutSelectMode);
    defineKey(Qt::Key_D, &SkeletonGraphicsWidget::shortcutPaintMode);
    defineKey(Qt::ALT + Qt::Key_Minus, &SkeletonGraphicsWidget::shortcutZoomRenderedModelByMinus10);
    defineKey(Qt::Key_Minus, &SkeletonGraphicsWidget::shortcutZoomSelectedByMinus1);
    defineKey(Qt::ALT + Qt::Key_Equal, &SkeletonGraphicsWidget::shortcutZoomRenderedModelBy10);
    defineKey(Qt::Key_Equal, &SkeletonGraphicsWidget::shortcutZoomSelectedBy1);
    defineKey(Qt::Key_Comma, &SkeletonGraphicsWidget::shortcutRotateSelectedByMinus1);
    defineKey(Qt::Key_Period, &SkeletonGraphicsWidget::shortcutRotateSelectedBy1);
    defineKey(Qt::Key_Left, &SkeletonGraphicsWidget::shortcutMoveSelectedToLeft);
    defineKey(Qt::Key_Right, &SkeletonGraphicsWidget::shortcutMoveSelectedToRight);
    defineKey(Qt::Key_Up, &SkeletonGraphicsWidget::shortcutMoveSelectedToUp);
    defineKey(Qt::Key_Down, &SkeletonGraphicsWidget::shortcutMoveSelectedToDown);
    defineKey(Qt::Key_BracketLeft, &SkeletonGraphicsWidget::shortcutScaleSelectedByMinus1);
    defineKey(Qt::Key_BracketRight, &SkeletonGraphicsWidget::shortcutScaleSelectedBy1);
    defineKey(Qt::Key_E, &SkeletonGraphicsWidget::shortcutSwitchProfileOnSelected);
    defineKey(Qt::Key_H, &SkeletonGraphicsWidget::shortcutShowOrHideSelectedPart);
    defineKey(Qt::Key_J, &SkeletonGraphicsWidget::shortcutEnableOrDisableSelectedPart);
    defineKey(Qt::Key_L, &SkeletonGraphicsWidget::shortcutLockOrUnlockSelectedPart);
    defineKey(Qt::Key_M, &SkeletonGraphicsWidget::shortcutXmirrorOnOrOffSelectedPart);
    defineKey(Qt::Key_B, &SkeletonGraphicsWidget::shortcutSubdivedOrNotSelectedPart);
    defineKey(Qt::Key_U, &SkeletonGraphicsWidget::shortcutRoundEndOrNotSelectedPart);
    defineKey(Qt::Key_W, &SkeletonGraphicsWidget::shortcutToggleWireframe);
    defineKey(Qt::Key_F, &SkeletonGraphicsWidget::shortcutCheckPartComponent);
    defineKey(Qt::Key_C, &SkeletonGraphicsWidget::shortcutChamferedOrNotSelectedPart);
    defineKey(Qt::Key_O, &SkeletonGraphicsWidget::shortcutToggleFlatShading);
    defineKey(Qt::Key_R, &SkeletonGraphicsWidget::shortcutToggleRotation);
}
