#ifndef DUST3D_APPLICATION_BACKGROUND_TASK_GROUP_H_
#define DUST3D_APPLICATION_BACKGROUND_TASK_GROUP_H_

#include <QPointer>
#include <QThread>
#include <vector>
#include <algorithm>

// Accessed by the owning GUI thread. Join every task before releasing its inputs,
// workers, or widgets. QPointer also covers threads already deleted by deleteLater.
class BackgroundTaskGroup {
public:
    BackgroundTaskGroup() = default;
    BackgroundTaskGroup(const BackgroundTaskGroup&) = delete;
    BackgroundTaskGroup& operator=(const BackgroundTaskGroup&) = delete;
    ~BackgroundTaskGroup() { waitForDone(); }

    // Pass worker only when it has no other owner (e.g. an export lambda).
    void add(QThread* thread, QObject* worker = nullptr)
    {
        m_tasks.erase(std::remove_if(m_tasks.begin(), m_tasks.end(),
            [](const Task& task) { return !task.thread && !task.worker; }), m_tasks.end());
        m_tasks.push_back({ thread, worker });
    }

    void waitForDone()
    {
        for (const auto& task : m_tasks) {
            if (task.thread) {
                task.thread->requestInterruption();
                task.thread->quit();
            }
        }
        for (const auto& task : m_tasks) {
            if (task.thread)
                task.thread->wait();
        }
        for (const auto& task : m_tasks) {
            delete task.worker.data();
            delete task.thread.data();
        }
        m_tasks.clear();
    }

private:
    struct Task {
        QPointer<QThread> thread;
        QPointer<QObject> worker;
    };
    std::vector<Task> m_tasks;
};

#endif
