#include "preferences.h"
#include "util.h"

Preferences &Preferences::instance()
{
    static Preferences *s_preferences = nullptr;
    if (nullptr == s_preferences) {
        s_preferences = new Preferences;
    }
    return *s_preferences;
}

void Preferences::loadDefault()
{
    m_componentCombineMode = CombineMode::Normal;
    m_partColor = Qt::white;
    m_flatShading = false;
    m_toonShading = false;
    m_textureSize = 1024;
}

Preferences::Preferences()
{
    loadDefault();
    {
        QString value = m_settings.value("componentCombineMode").toString();
        if (!value.isEmpty())
            m_componentCombineMode = CombineModeFromString(value.toUtf8().constData());
    }
    {
        QString value = m_settings.value("partColor").toString();
        if (!value.isEmpty())
            m_partColor = QColor(value);
    }
    {
        QString value = m_settings.value("flatShading").toString();
        if (value.isEmpty())
            m_flatShading = false;
        else
            m_flatShading = isTrueValueString(value);
    }
    {
        QString value = m_settings.value("toonShading").toString();
        if (value.isEmpty())
            m_toonShading = false;
        else
            m_toonShading = isTrueValueString(value);
    }
    {
        QString value = m_settings.value("textureSize").toString();
        if (!value.isEmpty())
            m_textureSize = value.toInt();
    }
}

CombineMode Preferences::componentCombineMode() const
{
    return m_componentCombineMode;
}

const QColor &Preferences::partColor() const
{
    return m_partColor;
}

bool Preferences::flatShading() const
{
    return m_flatShading;
}

bool Preferences::toonShading() const
{
    return m_toonShading;
}

int Preferences::textureSize() const
{
    return m_textureSize;
}

void Preferences::setComponentCombineMode(CombineMode mode)
{
    if (m_componentCombineMode == mode)
        return;
    m_componentCombineMode = mode;
    m_settings.setValue("componentCombineMode", CombineModeToString(m_componentCombineMode));
    emit componentCombineModeChanged();
}

void Preferences::setPartColor(const QColor &color)
{
    if (m_partColor == color)
        return;
    m_partColor = color;
    m_settings.setValue("partColor", color.name());
    emit partColorChanged();
}

void Preferences::setFlatShading(bool flatShading)
{
    if (m_flatShading == flatShading)
        return;
    m_flatShading = flatShading;
    m_settings.setValue("flatShading", flatShading ? "true" : "false");
    emit flatShadingChanged();
}

void Preferences::setToonShading(bool toonShading)
{
    if (m_toonShading == toonShading)
        return;
    m_toonShading = toonShading;
    m_settings.setValue("toonShading", toonShading ? "true" : "false");
    emit toonShadingChanged();
}

void Preferences::setTextureSize(int textureSize)
{
    if (m_textureSize == textureSize)
        return;
    m_textureSize = textureSize;
    m_settings.setValue("textureSize", QString::number(m_textureSize));
    emit textureSizeChanged();
}

QSize Preferences::documentWindowSize() const
{
    return m_settings.value("documentWindowSize", QSize()).toSize();
}

void Preferences::setDocumentWindowSize(const QSize& size)
{
    m_settings.setValue("documentWindowSize", size);
}

void Preferences::reset()
{
    m_settings.clear();
    loadDefault();
    emit componentCombineModeChanged();
    emit partColorChanged();
    emit flatShadingChanged();
    emit toonShadingChanged();
    emit textureSizeChanged();
}
