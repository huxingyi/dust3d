#include "turnaround_overlay_widget.h"
#include "document_window.h"
#include "preferences.h"
#include "preview_overlay_controller.h"
#include "scene_widget.h"
#include "theme.h"
#include <QAction>
#include <QDesktopServices>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QRandomGenerator>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

TurnaroundOverlayWidget::TurnaroundOverlayWidget(QWidget* parent, DocumentWindow* window)
    : QWidget(parent)
    , m_window(window)
{
    setAcceptDrops(true);
    setObjectName("turnaroundShortcutsOverlay");
    setAttribute(Qt::WA_TranslucentBackground);

    const QString overlayBackground = Theme::darkBackground.name();
    const QString buttonBackground = Theme::separator.name();
    const QString buttonHover = Theme::buttonDimmed.name();
    const QString titleColor = Theme::white.name();
    const QString subtitleColor = Theme::buttonDimmed.name();

    QVBoxLayout* overlayLayout = new QVBoxLayout(this);
    overlayLayout->setContentsMargins(16, 16, 16, 16);
    overlayLayout->setSpacing(24);
    overlayLayout->setAlignment(Qt::AlignCenter);

    setStyleSheet(QString()
        + "background-color: " + overlayBackground + ";"
        + "border-radius: 14px;");

    m_cardTitleLabel = new QLabel(tr("Get started"), this);
    m_cardTitleLabel->setStyleSheet("color: " + subtitleColor + "; font-size: 11px;");
    m_cardTitleLabel->setWordWrap(false);
    m_cardTitleLabel->setAlignment(Qt::AlignLeft);
    m_cardTitleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_cardTitleLabel->setContentsMargins(0, 12, 0, 12);

    auto createOverlayButton = [&](const QString& text, const QIcon& icon) {
        QToolButton* button = new QToolButton(this);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setAutoRaise(false);
        button->setIcon(icon);
        button->setIconSize(QSize(Theme::miniIconSize * 2 / 3, Theme::miniIconSize * 2 / 3));
        button->setText(text);
        button->setFixedHeight(Theme::miniIconSize + 6);
        button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        button->setStyleSheet(QString()
            + "QToolButton {"
            + " color: white;"
            + " background-color: transparent;"
            + " border: 1px solid rgba(255, 255, 255, 0.03);"
            + " border-radius: 6px;"
            + " font-size: 11px;"
            + " font-weight: 600;"
            + " line-height: 16px;"
            + " padding: 4px 8px;"
            + " text-align: left;"
            + "}"
            + "QToolButton:hover { background-color: rgba(255, 255, 255, 0.04); }");
        return button;
    };

    QVariantMap openIconOptions;
    openIconOptions.insert("color", Theme::blue.name());
    QIcon openIcon = Theme::awesome()->icon(fa::folderopen, openIconOptions);

    QVariantMap startIconOptions;
    startIconOptions.insert("color", Theme::green.name());
    QIcon startIcon = Theme::awesome()->icon(fa::plus, startIconOptions);

    QVariantMap donateIconOptions;
    donateIconOptions.insert("color", Theme::red.name());
    QIcon donateIcon = Theme::awesome()->icon(fa::heart, donateIconOptions);

    QToolButton* startButton = createOverlayButton(tr("Start"), startIcon);
    QToolButton* openButton = createOverlayButton(tr("Open"), openIcon);
    QMenu* openRecentFileMenu = new QMenu;
    openRecentFileMenu->setStyleSheet(QString()
        + "QMenu { color: white; background-color: " + Theme::altDarkBackground.name() + "; }"
        + "QMenu::item { padding: 4px 16px; }"
        + "QMenu::item:selected { color: " + Theme::red.name() + "; }");

    QVariantMap recentIconOptions;
    recentIconOptions.insert("color", Theme::blue.name());
    QIcon recentIcon = Theme::awesome()->icon(fa::history, recentIconOptions);

    QToolButton* recentButton = new QToolButton(this);
    recentButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    recentButton->setAutoRaise(false);
    recentButton->setIcon(recentIcon);
    recentButton->setIconSize(QSize(Theme::miniIconSize * 2 / 3, Theme::miniIconSize * 2 / 3));
    recentButton->setFixedSize(Theme::miniIconSize + 10, Theme::miniIconSize + 6);
    recentButton->setStyleSheet(QString()
        + "QToolButton {"
        + " color: white;"
        + " background-color: transparent;"
        + " border: 1px solid rgba(255, 255, 255, 0.03);"
        + " border-radius: 6px;"
        + " font-size: 11px;"
        + " line-height: 16px;"
        + " padding: 0px;"
        + " text-align: left;"
        + "}"
        + "QToolButton:hover { background-color: rgba(255, 255, 255, 0.04); }");
    connect(recentButton, &QToolButton::clicked, this, [openRecentFileMenu, recentButton]() {
        openRecentFileMenu->popup(recentButton->mapToGlobal(QPoint(0, recentButton->height())));
    });

    QWidget* openGroup = new QWidget(this);
    QHBoxLayout* openGroupLayout = new QHBoxLayout(openGroup);
    openGroupLayout->setContentsMargins(0, 0, 0, 0);
    openGroupLayout->setSpacing(4);
    openGroupLayout->addWidget(openButton, 1);
    openGroupLayout->addWidget(recentButton, 0);

    connect(openRecentFileMenu, &QMenu::aboutToShow, this, [=]() {
        openRecentFileMenu->clear();
        const QStringList files = Preferences::instance().recentFileList();
        if (files.isEmpty()) {
            QAction* noneAction = new QAction(tr("No recent files"), openRecentFileMenu);
            noneAction->setEnabled(false);
            openRecentFileMenu->addAction(noneAction);
            return;
        }
        for (int i = 0; i < files.size(); ++i) {
            QString text = tr("%1 %2").arg(i + 1).arg(strippedName(files[i]));
            QAction* action = new QAction(text, openRecentFileMenu);
            action->setData(files[i]);
            connect(action, &QAction::triggered, m_window, &DocumentWindow::openRecentFile, Qt::QueuedConnection);
            openRecentFileMenu->addAction(action);
        }
    });
    QToolButton* donateButton = createOverlayButton(tr("Donate"), donateIcon);
    connect(donateButton, &QToolButton::clicked, this, [=]() {
        QDesktopServices::openUrl(QUrl("https://fund.dust3d.org"));
    });

    QHBoxLayout* buttonRowLayout = new QHBoxLayout;
    buttonRowLayout->setContentsMargins(0, 0, 0, 0);
    buttonRowLayout->addWidget(startButton);
    buttonRowLayout->addStretch(1);
    buttonRowLayout->addWidget(openGroup);
    buttonRowLayout->addStretch(1);
    buttonRowLayout->addWidget(donateButton);

    overlayLayout->addLayout(buttonRowLayout);
    overlayLayout->addWidget(m_cardTitleLabel, 0, Qt::AlignLeft);

    QWidget* previewCard = new QWidget(this);
    QVBoxLayout* previewCardLayout = new QVBoxLayout(previewCard);
    previewCardLayout->setContentsMargins(0, 0, 0, 0);
    previewCardLayout->setSpacing(0);

    m_previewWidget = new SceneWidget(previewCard);
    m_previewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_previewWidget->setMoveAndZoomByWindow(false);
    m_previewWidget->toggleWireframe();
    previewCardLayout->addWidget(m_previewWidget);

    overlayLayout->addWidget(previewCard, 1);

    connect(startButton, &QToolButton::clicked, m_window, &DocumentWindow::changeTurnaround);
    connect(openButton, &QToolButton::clicked, m_window, &DocumentWindow::open);

    const QStringList previewResourcePaths = {
        ":/resources/preview_model_first.xml",
        ":/resources/preview_model_second.xml",
        ":/resources/preview_model_third.xml",
        ":/resources/preview_model_fourth.xml"
    };
    for (int i = 0; i < previewResourcePaths.size(); ++i) {
        PreviewOverlayController* controller = new PreviewOverlayController(m_previewWidget, previewResourcePaths[i], i, this);
        m_previewOverlayControllers.push_back(controller);
    }
}

void TurnaroundOverlayWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    for (PreviewOverlayController* controller : m_previewOverlayControllers) {
        if (controller)
            controller->start();
    }
    if (m_previewWidget)
        m_previewWidget->resumeSimulation();
}

void TurnaroundOverlayWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    for (PreviewOverlayController* controller : m_previewOverlayControllers) {
        if (controller)
            controller->stop();
    }
    if (m_previewWidget)
        m_previewWidget->stopSimulation();
}

void TurnaroundOverlayWidget::setCardTitle(const QString& text)
{
    if (m_cardTitleLabel)
        m_cardTitleLabel->setText(text);
}

void TurnaroundOverlayWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (canAcceptDrop(event->mimeData())) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void TurnaroundOverlayWidget::dragMoveEvent(QDragMoveEvent* event)
{
    if (canAcceptDrop(event->mimeData())) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void TurnaroundOverlayWidget::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }

    QStringList imageFiles;
    QString ds3File;
    for (const auto& url : event->mimeData()->urls()) {
        QString filePath = url.toLocalFile();
        QString lower = filePath.toLower();
        if (lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".bmp")) {
            imageFiles.append(filePath);
            continue;
        }
        if (lower.endsWith(".ds3")) {
            ds3File = filePath;
        }
    }

    if (!imageFiles.isEmpty()) {
        if (m_window)
            m_window->loadTurnaroundImageFiles(imageFiles);
        event->acceptProposedAction();
        return;
    }

    if (!ds3File.isEmpty()) {
        if (m_window)
            m_window->openDroppedDs3File(ds3File);
        event->acceptProposedAction();
        return;
    }

    event->ignore();
}

bool TurnaroundOverlayWidget::canAcceptDrop(const QMimeData* mimeData)
{
    if (!mimeData->hasUrls())
        return false;

    for (const auto& url : mimeData->urls()) {
        QString lower = url.toLocalFile().toLower();
        if (lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".bmp") || lower.endsWith(".ds3"))
            return true;
    }
    return false;
}

QString TurnaroundOverlayWidget::strippedName(const QString& fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}
