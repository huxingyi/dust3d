#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include "ds3file.h"

QString Ds3FileReader::m_applicationName = QString("DUST3D");
QString Ds3FileReader::m_fileFormatVersion = QString("1.0");
QString Ds3FileReader::m_headFormat = QString("xml");

QString Ds3FileReader::readFirstLine()
{
    QString firstLine;
    QFile file(m_filename);
    if (file.open(QIODevice::ReadOnly)) {
       QTextStream in(&file);
       if (!in.atEnd()) {
          firstLine = in.readLine();
       }
       file.close();
    }
    return firstLine;
}

Ds3FileReader::Ds3FileReader(const QString &filename) :
    m_headerIsGood(false),
    m_binaryOffset(0)
{
    m_filename = filename;
    QString firstLine = readFirstLine();
    QStringList tokens = firstLine.split(" ");
    if (tokens.length() < 4) {
        return;
    }
    if (tokens[0] != Ds3FileReader::m_applicationName) {
        return;
    }
    if (tokens[1] != Ds3FileReader::m_fileFormatVersion) {
        return;
    }
    if (tokens[2] != Ds3FileReader::m_headFormat) {
        return;
    }
    m_binaryOffset = tokens[3].toLongLong();
    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QString header = QString::fromUtf8(file.read(m_binaryOffset).constData()).mid(firstLine.size()).trimmed();
    QXmlStreamReader xml(header);
    bool ds3TagEntered = false;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "ds3") {
                ds3TagEntered = true;
                m_headerIsGood = true;
            } else {
                if (ds3TagEntered) {
                    Ds3ReaderItem readerItem;
                    readerItem.type = xml.name().toString().trimmed();
                    readerItem.name = xml.attributes().value("name").toString().trimmed();
                    readerItem.offset = xml.attributes().value("offset").toLongLong();
                    readerItem.size = xml.attributes().value("size").toLongLong();
                    m_items.push_back(readerItem);
                    m_itemsMap[readerItem.name] = readerItem;
                }
            }
        } else if (xml.isEndElement()) {
            if (xml.name() == "ds3") {
                ds3TagEntered = false;
            }
        }
    }
}

void Ds3FileReader::loadItem(const QString &name, QByteArray *byteArray)
{
    byteArray->clear();
    if (!m_headerIsGood)
        return;
    if (m_itemsMap.find(name) == m_itemsMap.end()) {
        return;
    }
    Ds3ReaderItem readerItem = m_itemsMap[name];
    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    if (!file.seek(m_binaryOffset + readerItem.offset)) {
        return;
    }
    *byteArray = file.read(readerItem.size);
}

const QList<Ds3ReaderItem> &Ds3FileReader::items()
{
    return m_items;
}

bool Ds3FileWriter::add(const QString &name, const QString &type, const QByteArray *byteArray)
{
    if (m_itemsMap.find(name) != m_itemsMap.end()) {
        return false;
    }
    Ds3WriterItem writerItem;
    writerItem.type = type;
    writerItem.name = name;
    writerItem.byteArray = *byteArray;
    m_itemsMap[name] = writerItem;
    m_items.push_back(writerItem);
    return true;
}

bool Ds3FileWriter::save(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QByteArray headerXml;
    {
        QXmlStreamWriter stream(&headerXml);
        stream.setAutoFormatting(true);
        stream.writeStartDocument();
        stream.writeStartElement("ds3");
        
        long long offset = 0;
        for (int i = 0; i < m_items.size(); i++) {
            Ds3WriterItem *writerItem = &m_items[i];
            stream.writeStartElement(writerItem->type);
                stream.writeAttribute("name", QString("%1").arg(writerItem->name));
                stream.writeAttribute("offset", QString("%1").arg(offset));
                stream.writeAttribute("size", QString("%1").arg(writerItem->byteArray.size()));
                offset += writerItem->byteArray.size();
            stream.writeEndElement();
        }
        
        stream.writeEndElement();
        stream.writeEndDocument();
    }
    char firstLine[1024];
    int firstLineSizeExcludeSizeSelf = sprintf(firstLine, "%s %s %s ",
        Ds3FileReader::m_applicationName.toUtf8().constData(),
        Ds3FileReader::m_fileFormatVersion.toUtf8().constData(),
        Ds3FileReader::m_headFormat.toUtf8().constData());
    unsigned int headerSize = firstLineSizeExcludeSizeSelf + 12 + headerXml.size();
    char headerSizeString[100] = {0};
    sprintf(headerSizeString, "%010u\r\n", headerSize);
    file.write(firstLine, firstLineSizeExcludeSizeSelf);
    file.write(headerSizeString, strlen(headerSizeString));
    file.write(headerXml);
    for (int i = 0; i < m_items.size(); i++) {
        Ds3WriterItem *writerItem = &m_items[i];
        file.write(writerItem->byteArray);
    }
    
    return true;
}

