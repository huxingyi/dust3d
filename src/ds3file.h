#ifndef DS3_FILE_H
#define DS3_FILE_H
#include <QObject>
#include <QString>
#include <QByteArray>
#include <map>

/*
DUST3D 1.0 xml 12345
<?xml version="1.0" encoding="UTF-8"?>
<ds3>
    <model name="ant.xml" offset="0" size="1024"/>
    <asset name="ant.jpg" offset="1024" size="279306"/>
</ds3>
... Binary content ...
*/

class Ds3ReaderItem
{
public:
    QString type;
    QString name;
    long long offset;
    long long size;
};

class Ds3FileReader : public QObject
{
    Q_OBJECT
public:
    Ds3FileReader(const QString &filename);
    void loadItem(const QString &name, QByteArray *byteArray);
    const QList<Ds3ReaderItem> &items();
    static QString m_applicationName;
    static QString m_fileFormatVersion;
    static QString m_headFormat;
private:
    std::map<QString, Ds3ReaderItem> m_itemsMap;
    QList<Ds3ReaderItem> m_items;
    QString m_filename;
private:
    QString readFirstLine();
    bool m_headerIsGood;
    long long m_binaryOffset;
};

class Ds3WriterItem
{
public:
    QString type;
    QString name;
    QByteArray byteArray;
};

class Ds3FileWriter : public QObject
{
    Q_OBJECT
public:
    bool add(const QString &name, const QString &type, const QByteArray *byteArray);
    bool save(const QString &filename);
private:
    std::map<QString, Ds3WriterItem> m_itemsMap;
    QList<Ds3WriterItem> m_items;
    QString m_filename;
};

#endif
