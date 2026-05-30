#ifndef DUST3D_APPLICATION_STEPS_REPLAY_WINDOW_H_
#define DUST3D_APPLICATION_STEPS_REPLAY_WINDOW_H_

#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QWidget>
#include <dust3d/base/uuid.h>
#include <functional>
#include <vector>

class Document;
class DocumentWindow;
class ComponentPropertyWidget;

class StepsReplayWindow : public QWidget {
    Q_OBJECT
public:
    StepsReplayWindow(Document* sourceDocument, QWidget* parent = nullptr);
    ~StepsReplayWindow();

private slots:
    void executeNextStep();
    void togglePlayPause();
    void speedChanged(int value);
    void onRigReady();

private:
    struct ReplayStep {
        QString title;
        std::function<void()> execute;
        bool waitForRig = false;
    };

    void buildSteps();
    void updateStepDisplay();
    void positionSelf();
    void closePropertyWidget();
    dust3d::Uuid findDemoPartId(const dust3d::Uuid& sourcePartId);
    dust3d::Uuid findDemoComponentId(const dust3d::Uuid& demoPartId);
    dust3d::Uuid findDemoEdgeByNodes(const dust3d::Uuid& nodeId1, const dust3d::Uuid& nodeId2);
    void showPropertyWidgetForComponent(const dust3d::Uuid& demoComponentId);

    Document* m_sourceDocument = nullptr;
    DocumentWindow* m_demoWindow = nullptr;
    Document* m_demoDocument = nullptr;

    QLabel* m_stepTitleLabel = nullptr;
    QLabel* m_stepProgressLabel = nullptr;
    QPushButton* m_nextButton = nullptr;
    QPushButton* m_playPauseButton = nullptr;
    QSlider* m_speedSlider = nullptr;
    QLabel* m_speedLabel = nullptr;
    QTimer* m_autoPlayTimer = nullptr;
    bool m_playing = false;
    bool m_waitingForRig = false;

    ComponentPropertyWidget* m_currentPropertyWidget = nullptr;

    std::vector<ReplayStep> m_steps;
    int m_currentStep = -1;
};

#endif
