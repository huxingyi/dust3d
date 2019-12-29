#ifndef PREFERENCES_H
#define PREFERENCES_H
#include <QSettings>
#include <QColor>
#include <QSize>
#include "combinemode.h"

class Preferences : public QObject
{
    Q_OBJECT
public:
    static Preferences &instance();
    Preferences();
    CombineMode componentCombineMode() const;
    const QColor &partColor() const;
    bool flatShading() const;
    QSize documentWindowSize() const;
    void setDocumentWindowSize(const QSize&);
signals:
    void componentCombineModeChanged();
    void partColorChanged();
    void flatShadingChanged();
public slots:
    void setComponentCombineMode(CombineMode mode);
    void setPartColor(const QColor &color);
    void setFlatShading(bool flatShading);
    void reset();
private:
    CombineMode m_componentCombineMode;
    QColor m_partColor;
    bool m_flatShading;
    QSettings m_settings;
private:
    void loadDefault();
};

#endif
