#ifndef DUST3D_UPDATES_CHECKER_H
#define DUST3D_UPDATES_CHECKER_H
#include <QObject>
#include <QtNetwork>

class UpdatesChecker : public QObject
{
    Q_OBJECT
signals:
    void finished();
public:
    struct UpdateItem
    {
        QString forTags;
        QString version;
        QString humanVersion;
        QString descriptionUrl;
    };
    
    UpdatesChecker();
    void start();
    bool isLatest() const;
    bool hasError() const;
    const QString &message() const;
    const UpdateItem &matchedUpdateItem() const;
private slots:
    void downloadFinished(QNetworkReply *reply);
private:
    QNetworkAccessManager m_networkAccessManager;
    bool m_isLatest = false;
    QString m_message;
    QString m_latestUrl;
    bool m_hasError = true;
    UpdateItem m_matchedUpdateItem;
    
    bool parseUpdateInfoXml(const QByteArray &updateInfoXml, std::vector<UpdateItem> *updateItems);
    static bool isVersionLessThan(const QString &version, const QString &compareWith);
};

#endif
