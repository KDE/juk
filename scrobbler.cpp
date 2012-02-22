#include "scrobbler.h"
#include <kglobal.h>
#include <kconfiggroup.h>
#include <QDir>
#include <KDebug>
#include <QCryptographicHash>
#include <KSharedConfig>
#include <KSharedPtr>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>
#include "tag.h"

Scrobbler::Scrobbler(QObject* parent): QObject(parent)
{
    KConfigGroup config(KGlobal::config(), "Scrobbling");
    m_sessionKey = config.readEntry("SessionKey", "");//TODO: use kwallet
    if (m_sessionKey.isEmpty())
        getAuthToken();
}

Scrobbler::~Scrobbler()
{
}

QByteArray Scrobbler::md5(QByteArray data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex().rightJustified(32, '0').toLower();
}


void Scrobbler::sign(QMap< QString, QString >& params)
{
    params["api_key"] = "3e6ecbd7284883089e8f2b5b53b0aecd";
    QString s;
    QMapIterator<QString, QString> i( params );
    while (i.hasNext()) {
        i.next();
        s += i.key() + i.value();
    }
    s += "2cab3957b1f70d485e9815ac1ac94096"; //shared secret
    params["api_sig"] = md5(s.toUtf8());
}


void Scrobbler::getAuthToken()
{   
    kDebug() << "Getting new auth token...";
    
    KConfigGroup config(KGlobal::config(), "Scrobbling");
    QString username = config.readEntry("Username", "");//TODO: use kwallet
    QString password = config.readEntry("Password", "");//TODO: use kwallet
    if (username.isEmpty() || password.isEmpty())
        return;
    
    QByteArray authToken = md5((username + md5(password.toUtf8() )).toUtf8());
    
    QMap<QString, QString> params;
    params["method"] = "auth.getMobileSession";

    params["authToken"] = authToken;
    params["username"] = username;
    QNetworkAccessManager *qnam = new QNetworkAccessManager(this);
    QUrl url("http://ws.audioscrobbler.com/2.0/?");
    
    sign(params);
    
    foreach(QString key, params.keys()) {
        url.addQueryItem(key, params[key]);
    }

    QNetworkReply *reply = qnam->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(handleAuthenticationReply()));
}

void Scrobbler::handleAuthenticationReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    qWarning() << "got authentication reply";
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << reply->errorString();
        return;
    }
    QDomDocument doc;
    QByteArray data = reply->readAll();
    qWarning() << data;
    doc.setContent(data);
    QDomElement el = doc.documentElement();
    m_sessionKey = el.firstChildElement("session").firstChildElement("key").text();
    
    KConfigGroup config(KGlobal::config(), "Scrobbling");
    config.writeEntry("SessionKey", m_sessionKey);
    return;
}


void Scrobbler::nowPlaying(const FileHandle& file)
{
    if (m_sessionKey.isEmpty()) {
        getAuthToken();
        return;
    }

    qWarning() << "Now playing" << file.tag()->title();
    
    QMap<QString, QString> params;
    params["method"] = "track.updateNowPlaying";
    params["sk"] = m_sessionKey;
    
    params["track"] = file.tag()->title();
    params["artist"] = file.tag()->artist();
    params["album"] = file.tag()->album();
    params["trackNumber"] = QString::number(file.tag()->track());
    params["duration"] = QString::number(file.tag()->seconds());
    
    sign(params);
    post(params);
    
    m_file = file;
    m_startedPlaying = QDateTime::currentMSecsSinceEpoch() / 1000;
}

void Scrobbler::scrobble()
{
    if (m_sessionKey.isEmpty()) {
        getAuthToken();
        return;
    }
    
    qWarning() << "Scrobbling" << m_file.tag()->title();
    
    QMap<QString, QString> params;
    params["method"] = "track.scrobble";
    params["sk"] = m_sessionKey;
    
    params["timestamp"] = QString::number(m_startedPlaying);
    
    params["track"] = m_file.tag()->title();
    params["artist"] = m_file.tag()->artist();
    params["album"] = m_file.tag()->album();
    params["trackNumber"] = QString::number(m_file.tag()->track());
    params["duration"] = QString::number(m_file.tag()->seconds());
    
    sign(params);
    post(params);
}

void Scrobbler::post(QMap<QString, QString> &params)
{
    QNetworkAccessManager *qnam = new QNetworkAccessManager(this);
    QUrl url("http://ws.audioscrobbler.com/2.0/");
    
    QByteArray data;
    foreach(QString key, params.keys()) {
        data += QUrl::toPercentEncoding(key) + "=" + QUrl::toPercentEncoding(params[key]) + "&";
    }

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = qnam->post(req, data);
    connect(reply, SIGNAL(finished()), this, SLOT(handleResults()));
}


void Scrobbler::handleResults()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray data = reply->readAll();
    qWarning() << data;
    if (data.contains("code=\"9\"")) // We need a new token
        getAuthToken();
}
