#ifndef DUST3D_APPLICATION_PREFERENCES_H_
#define DUST3D_APPLICATION_PREFERENCES_H_

#include <QSettings>
#include <QSize>
#include <QStringList>

class Preferences : public QObject {
    Q_OBJECT
public:
    static Preferences& instance();
    Preferences();
    QSize documentWindowSize() const;
    void setDocumentWindowSize(const QSize&);
    QStringList recentFileList() const;
    int maxRecentFiles() const;
public slots:
    void setCurrentFile(const QString& fileName);
    void reset();

private:
    QSettings m_settings;
    void loadDefault();
};

#endif
