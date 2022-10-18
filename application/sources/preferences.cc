#include "preferences.h"

#define MAX_RECENT_FILES 7

Preferences& Preferences::instance()
{
    static Preferences* s_preferences = nullptr;
    if (nullptr == s_preferences) {
        s_preferences = new Preferences;
    }
    return *s_preferences;
}

void Preferences::loadDefault()
{
}

Preferences::Preferences()
{
    loadDefault();
}

QSize Preferences::documentWindowSize() const
{
    return m_settings.value("documentWindowSize", QSize()).toSize();
}

void Preferences::setDocumentWindowSize(const QSize& size)
{
    m_settings.setValue("documentWindowSize", size);
}

QStringList Preferences::recentFileList() const
{
    return m_settings.value("recentFileList").toStringList();
}

int Preferences::maxRecentFiles() const
{
    return MAX_RECENT_FILES;
}

void Preferences::setCurrentFile(const QString& fileName)
{
    QStringList files = m_settings.value("recentFileList").toStringList();

    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MAX_RECENT_FILES)
        files.removeLast();

    m_settings.setValue("recentFileList", files);
}

void Preferences::reset()
{
    auto files = m_settings.value("recentFileList").toStringList();
    m_settings.clear();
    m_settings.setValue("recentFileList", files);

    loadDefault();
}
