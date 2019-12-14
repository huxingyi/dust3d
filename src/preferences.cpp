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
    m_flatShading = true;
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

void Preferences::reset()
{
    m_settings.clear();
    loadDefault();
    emit componentCombineModeChanged();
    emit partColorChanged();
    emit flatShadingChanged();
}
