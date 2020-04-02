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
    bool toonShading() const;
    QSize documentWindowSize() const;
    void setDocumentWindowSize(const QSize&);
    int textureSize() const;
signals:
    void componentCombineModeChanged();
    void partColorChanged();
    void flatShadingChanged();
    void toonShadingChanged();
    void textureSizeChanged();
public slots:
    void setComponentCombineMode(CombineMode mode);
    void setPartColor(const QColor &color);
    void setFlatShading(bool flatShading);
    void setToonShading(bool toonShading);
    void setTextureSize(int textureSize);
    void reset();
private:
    CombineMode m_componentCombineMode;
    QColor m_partColor;
    bool m_flatShading;
    bool m_toonShading;
    QSettings m_settings;
    int m_textureSize;
private:
    void loadDefault();
};

#endif
