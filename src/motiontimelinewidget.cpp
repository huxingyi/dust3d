#include <QAbstractItemView>
#include <QPalette>
#include <QMenu>
#include <QWidgetAction>
#include <QCheckBox>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include "motiontimelinewidget.h"
#include "motionclipwidget.h"
#include "theme.h"
#include "posewidget.h"
#include "motionwidget.h"

MotionTimelineWidget::MotionTimelineWidget(const Document *document, QWidget *parent) :
    QListWidget(parent),
    m_document(document)
{
    setSelectionMode(QAbstractItemView::NoSelection);
    setFocusPolicy(Qt::NoFocus);
    
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::transparent);
    palette.setColor(QPalette::Base, Qt::transparent);
    setPalette(palette);
    
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSpacing(0);
    setContentsMargins(0, 0, 0, 0);
    setFlow(QListWidget::LeftToRight);
    
    auto minHeight = MotionClipWidget::maxSize().height();
    setMinimumHeight(minHeight + 4);
    setMaximumHeight(minHeight + 4 + 20);
}

QSize MotionTimelineWidget::sizeHint() const
{
    return QSize(0, MotionClipWidget::maxSize().height() + 4);
}

const std::vector<MotionClip> &MotionTimelineWidget::clips()
{
    return m_clips;
}

void MotionTimelineWidget::setClips(std::vector<MotionClip> clips)
{
    m_clips = clips;
    if (m_currentSelectedIndex >= (int)m_clips.size())
        m_currentSelectedIndex = -1;
    reload();
}

void MotionTimelineWidget::addPose(QUuid poseId)
{
    MotionClip clip;
    clip.linkToId = poseId;
    clip.clipType = MotionClipType::Pose;
    clip.duration = 0;
    addClipAfterCurrentIndex(clip);
    emit clipsChanged();
    reload();
}

void MotionTimelineWidget::addClipAfterCurrentIndex(const MotionClip &clip)
{
    MotionClip interpolationClip;
    bool needPrependInterpolationClip = false;
    int afterIndex = m_currentSelectedIndex;
    if (-1 == afterIndex)
        afterIndex = m_clips.size() - 1;
    
    if (-1 != afterIndex) {
        if (m_clips[afterIndex].clipType == MotionClipType::Interpolation) {
            --afterIndex;
        }
    }
    
    if (clip.clipType == MotionClipType::Interpolation) {
        if (m_clips.empty())
            return;
        if (m_clips[m_clips.size() - 1].clipType == MotionClipType::Interpolation)
            return;
    } else {
        if (!m_clips.empty() && m_clips[m_clips.size() - 1].clipType != MotionClipType::Interpolation) {
            interpolationClip.interpolationType = InterpolationType::EaseInOutCubic;
            interpolationClip.clipType = MotionClipType::Interpolation;
            interpolationClip.duration = 1.0;
            needPrependInterpolationClip = true;
        }
    }
    
    if (-1 == afterIndex) {
        if (needPrependInterpolationClip)
            m_clips.push_back(interpolationClip);
        m_clips.push_back(clip);
    } else {
        if (needPrependInterpolationClip)
            m_clips.insert(m_clips.begin() + afterIndex + 1, interpolationClip);
        m_clips.insert(m_clips.begin() + afterIndex + 2, clip);
    }
}

void MotionTimelineWidget::addMotion(QUuid motionId)
{
    MotionClip clip;
    clip.linkToId = motionId;
    clip.clipType = MotionClipType::Motion;
    clip.duration = 0;
    addClipAfterCurrentIndex(clip);
    emit clipsChanged();
    reload();
}

void MotionTimelineWidget::addProceduralAnimation(ProceduralAnimation proceduralAnimation)
{
    MotionClip clip;
    clip.clipType = MotionClipType::ProceduralAnimation;
    clip.duration = 0;
    clip.proceduralAnimation = proceduralAnimation;
    addClipAfterCurrentIndex(clip);
    emit clipsChanged();
    reload();
}

void MotionTimelineWidget::setClipInterpolationType(int index, InterpolationType type)
{
    if (index >= (int)m_clips.size())
        return;
    
    if (m_clips[index].clipType != MotionClipType::Interpolation)
        return;
    
    m_clips[index].interpolationType = type;
    emit clipsChanged();
}

void MotionTimelineWidget::setClipDuration(int index, float duration)
{
    if (index >= (int)m_clips.size())
        return;
    
    if (m_clips[index].clipType == MotionClipType::Motion)
        return;
    
    m_clips[index].duration = duration;
    MotionClipWidget *widget = (MotionClipWidget *)itemWidget(item(index));
    widget->setClip(m_clips[index]);
    widget->reload();
    emit clipsChanged();
}

void MotionTimelineWidget::reload()
{
    clear();
    
    for (int row = 0; row < (int)m_clips.size(); ++row) {
        MotionClipWidget *widget = new MotionClipWidget(m_document);
        widget->setClip(m_clips[row]);
        connect(widget, &MotionClipWidget::modifyInterpolation, this, [=]() {
            showInterpolationSettingPopup(row, mapFromGlobal(QCursor::pos()));
        });
        QListWidgetItem *item = new QListWidgetItem(this);
        auto itemSize = widget->preferredSize();
        itemSize.setWidth(itemSize.width() + 2);
        itemSize.setHeight(itemSize.height() + 2);
        item->setSizeHint(itemSize);
        item->setData(Qt::UserRole, QVariant(row));
        item->setBackground(Theme::black);
        addItem(item);
        setItemWidget(item, widget);
        widget->reload();
        if (m_currentSelectedIndex == row)
            widget->updateCheckedState(true);
    }
}

void MotionTimelineWidget::showInterpolationSettingPopup(int clipIndex, const QPoint &pos)
{
    QMenu popupMenu;
    
    QWidget *popup = new QWidget;
    
    QCheckBox *bouncingBeginBox = new QCheckBox();
    bouncingBeginBox->setChecked(InterpolationIsBouncingBegin(clips()[clipIndex].interpolationType));
    connect(bouncingBeginBox, &QCheckBox::stateChanged, this, [=]() {
        bool bouncingBegin = bouncingBeginBox->isChecked();
        const auto &oldType = clips()[clipIndex].interpolationType;
        bool bouncingEnd = InterpolationIsBouncingEnd(oldType);
        InterpolationType newType = InterpolationMakeBouncingType(oldType, bouncingBegin, bouncingEnd);
        setClipInterpolationType(clipIndex, newType);
    });
    
    QCheckBox *bouncingEndBox = new QCheckBox();
    bouncingEndBox->setChecked(InterpolationIsBouncingEnd(clips()[clipIndex].interpolationType));
    connect(bouncingEndBox, &QCheckBox::stateChanged, this, [=]() {
        bool bouncingEnd = bouncingEndBox->isChecked();
        const auto &oldType = clips()[clipIndex].interpolationType;
        bool bouncingBegin = InterpolationIsBouncingBegin(oldType);
        InterpolationType newType = InterpolationMakeBouncingType(oldType, bouncingBegin, bouncingEnd);
        setClipInterpolationType(clipIndex, newType);
    });
    
    QDoubleSpinBox *durationEdit = new QDoubleSpinBox();
    durationEdit->setDecimals(2);
    durationEdit->setMaximum(60);
    durationEdit->setMinimum(0);
    durationEdit->setSingleStep(0.1);
    durationEdit->setValue(clips()[clipIndex].duration);
    
    connect(durationEdit, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, [=](double value) {
        setClipDuration(clipIndex, (float)value);
    });
    
    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow(tr("Bouncing Begin:"), bouncingBeginBox);
    formLayout->addRow(tr("Bouncing End:"), bouncingEndBox);
    formLayout->addRow(tr("Duration:"), durationEdit);
    
    popup->setLayout(formLayout);
    
    QWidgetAction *action = new QWidgetAction(this);
    action->setDefaultWidget(popup);
    
    popupMenu.addAction(action);
    
    popupMenu.exec(mapToGlobal(pos));
}

void MotionTimelineWidget::mousePressEvent(QMouseEvent *event)
{
    QListWidget::mousePressEvent(event);
    if (event->button() == Qt::RightButton) {
        showContextMenu(mapFromGlobal(event->globalPos()));
        return;
    }
    QModelIndex itemIndex = indexAt(event->pos());
    if (!itemIndex.isValid())
        return;
    if (event->button() == Qt::LeftButton) {
        int row = itemIndex.row();
        setCurrentIndex(row);
    }
}

void MotionTimelineWidget::setCurrentIndex(int index)
{
    if (m_currentSelectedIndex == index || index >= (int)m_clips.size())
        return;
    
    if (-1 != m_currentSelectedIndex) {
        MotionClipWidget *widget = (MotionClipWidget *)itemWidget(item(m_currentSelectedIndex));
        widget->updateCheckedState(false);
    }
    m_currentSelectedIndex = index;
    {
        MotionClipWidget *widget = (MotionClipWidget *)itemWidget(item(m_currentSelectedIndex));
        widget->updateCheckedState(true);
    }
}

void MotionTimelineWidget::removeClip(int index)
{
    if (index >= (int)m_clips.size())
        return;
    
    if (index == m_currentSelectedIndex) {
        if (index - 2 >= 0) {
            setCurrentIndex(index - 2);
        } else if (index + 2 < (int)m_clips.size()) {
            setCurrentIndex(index + 2);
            // We need remove one clip and the interpolation, so here we -2
            m_currentSelectedIndex -= 2;
        } else {
            m_currentSelectedIndex = -1;
        }
    }
    m_clips.erase(m_clips.begin() + index);
    if (index - 2 >= 0) {
        // Remove the interpolation before this clip
        m_clips.erase(m_clips.begin() + index - 1);
        
    } else if (index < (int)m_clips.size()) {
        // Remove the interpolation after this clip
        m_clips.erase(m_clips.begin() + index);
    }
    emit clipsChanged();
    reload();
}

void MotionTimelineWidget::showContextMenu(const QPoint &pos)
{
    QMenu contextMenu(this);
    
    QAction doubleDurationAction(tr("Double Duration"), this);
    if (-1 != m_currentSelectedIndex) {
        if (m_clips[m_currentSelectedIndex].clipType == MotionClipType::Interpolation) {
            connect(&doubleDurationAction, &QAction::triggered, [=]() {
                setClipDuration(m_currentSelectedIndex, m_clips[m_currentSelectedIndex].duration * 2);
            });
            contextMenu.addAction(&doubleDurationAction);
        }
    }
    
    QAction halveDurationAction(tr("Halve Duration"), this);
    if (-1 != m_currentSelectedIndex) {
        if (m_clips[m_currentSelectedIndex].clipType == MotionClipType::Interpolation) {
            connect(&halveDurationAction, &QAction::triggered, [=]() {
                setClipDuration(m_currentSelectedIndex, m_clips[m_currentSelectedIndex].duration / 2);
            });
            contextMenu.addAction(&halveDurationAction);
        }
    }
    
    QAction deleteAction(tr("Delete"), this);
    if (-1 != m_currentSelectedIndex) {
        if (m_clips[m_currentSelectedIndex].clipType != MotionClipType::Interpolation) {
            connect(&deleteAction, &QAction::triggered, [=]() {
                removeClip(m_currentSelectedIndex);
            });
            contextMenu.addAction(&deleteAction);
        }
    }
    
    contextMenu.exec(mapToGlobal(pos));
}
