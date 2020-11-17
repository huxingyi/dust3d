#ifndef PREFERENCES_H
#define PREFERENCES_H
#include <QSettings>
#include <QColor>
#include <QSize>
#include "combinemode.h"
#include "toonline.h"

class Preferences : public QObject
{
    Q_OBJECT
public:
    static Preferences &instance();
    Preferences();
    CombineMode componentCombineMode() const;
    const QColor &partColor() const;
    bool flatShading() const;
    bool scriptEnabled() const;
    bool toonShading() const;
    ToonLine toonLine() const;
    QSize documentWindowSize() const;
    void setDocumentWindowSize(const QSize&);
    int textureSize() const;
signals:
    void componentCombineModeChanged();
    void partColorChanged();
    void flatShadingChanged();
    void toonShadingChanged();
    void toonLineChanged();
    void textureSizeChanged();
    void scriptEnabledChanged();
public slots:
    void setComponentCombineMode(CombineMode mode);
    void setPartColor(const QColor &color);
    void setFlatShading(bool flatShading);
    void setToonShading(bool toonShading);
    void setToonLine(ToonLine toonLine);
    void setTextureSize(int textureSize);
    void setScriptEnabled(bool enabled);
    void reset();
private:
    CombineMode m_componentCombineMode;
    QColor m_partColor;
    bool m_flatShading;
    bool m_toonShading;
    ToonLine m_toonLine;
    QSettings m_settings;
    int m_textureSize;
    bool m_scriptEnabled;
private:
    void loadDefault();
};

#endif
