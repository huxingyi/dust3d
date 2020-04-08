#ifndef DUST3D_TOON_LINE_H
#define DUST3D_TOON_LINE_H
#include <QString>

enum class ToonLine
{
    WithoutLine = 0,
    WithLine,
    LineOnly,
    Count
};

ToonLine ToonLineFromString(const char *lineString);
#define IMPL_ToonLineFromString                                              \
ToonLine ToonLineFromString(const char *lineString)                          \
{                                                                            \
    QString line = lineString;                                               \
    if (line == "WithoutLine")                                               \
        return ToonLine::WithoutLine;                                        \
    if (line == "WithLine")                                                  \
        return ToonLine::WithLine;                                           \
    if (line == "LineOnly")                                                  \
        return ToonLine::LineOnly;                                           \
    return ToonLine::WithoutLine;                                            \
}
QString ToonLineToString(ToonLine toonLine);
#define IMPL_ToonLineToString                                                \
QString ToonLineToString(ToonLine toonLine)                                  \
{                                                                            \
    switch (toonLine) {                                                      \
        case ToonLine::WithoutLine:                                          \
            return "WithoutLine";                                            \
        case ToonLine::WithLine:                                             \
            return "WithLine";                                               \
        case ToonLine::LineOnly:                                             \
            return "LineOnly";                                               \
        default:                                                             \
            return "";                                                       \
    }                                                                        \
}
QString ToonLineToDispName(ToonLine toonLine);
#define IMPL_ToonLineToDispName                                              \
QString ToonLineToDispName(ToonLine toonLine)                                \
{                                                                            \
    switch (toonLine) {                                                      \
        case ToonLine::WithoutLine:                                          \
            return QObject::tr("Without Line");                              \
        case ToonLine::WithLine:                                             \
            return QObject::tr("With Line");                                 \
        case ToonLine::LineOnly:                                             \
            return QObject::tr("Line Only");                                 \
        default:                                                             \
            return QObject::tr("Without Line");                              \
    }                                                                        \
}

#endif
