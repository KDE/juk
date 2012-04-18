/***************************************************************************
    begin                : Tue Feb 21 2012
    copyright            : (C) 2012 by Martin Sandsmark
    email                : martin.sandsmark@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "scrobbler.h"

#include <QCryptographicHash>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>

#include <kglobal.h>
#include <kconfiggroup.h>
#include <KDebug>
#include <KSharedConfig>
#include <KSharedPtr>

#include "tag.h"

Scrobbler::Scrobbler(QObject* parent): QObject(parent)
{
    KConfigGroup config(KGlobal::config(), "Scrobbling");

    //TODO: use kwallet
    QString sessionKey = config.readEntry("SessionKey", "");

    if(sessionKey.isEmpty())
        getAuthToken();
}

Scrobbler::~Scrobbler()
{
}

QByteArray Scrobbler::md5(QByteArray data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5)
        .toHex().rightJustified(32, '0').toLower();
}


void Scrobbler::sign(QMap< QString, QString >& params)
{
    QString s;
    QMapIterator<QString, QString> i(params);

    while(i.hasNext()) {
        i.next();
        s += i.key() + i.value();
    }

    s += "2cab3957b1f70d485e9815ac1ac94096"; //shared secret

    params["api_key"] = "3e6ecbd7284883089e8f2b5b53b0aecd";
    params["api_sig"] = md5(s.toUtf8());
}

void Scrobbler::getAuthToken(QString username, QString password)
{
    QByteArray authToken = md5((username + md5(password.toUtf8())).toUtf8());

    QMap<QString, QString> params;
    params["method"]    = "auth.getMobileSession";
    params["authToken"] = authToken;
    params["username"]  = username;

    QNetworkAccessManager *qnam = new QNetworkAccessManager(this);
    QUrl url("http://ws.audioscrobbler.com/2.0/?");

    sign(params);

    foreach(QString key, params.keys()) {
        url.addQueryItem(key, params[key]);
    }

    QNetworkReply *reply = qnam->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(handleAuthenticationReply()));
}

void Scrobbler::getAuthToken()
{
    kDebug() << "Getting new auth token...";

    KConfigGroup config(KGlobal::config(), "Scrobbling");

    // TODO: use kwallet
    QString username = config.readEntry("Username", "");
    QString password = config.readEntry("Password", "");

    if(username.isEmpty() || password.isEmpty())
        return;

    getAuthToken(username, password);
}

void Scrobbler::handleAuthenticationReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    kDebug() << "got authentication reply";
    if(reply->error() != QNetworkReply::NoError) {
        emit invalidAuth();
        kWarning() << "Error while getting authentication reply" << reply->errorString();
        return;
    }

    QDomDocument doc;
    QByteArray data = reply->readAll();
    doc.setContent(data);

    QString sessionKey = doc.documentElement()
        .firstChildElement("session")
            .firstChildElement("key").text();

    if(sessionKey.isEmpty()) {
        emit invalidAuth();
        kWarning() << "Unable to get session key" << data;
        return;
    }

    KConfigGroup config(KGlobal::config(), "Scrobbling");
    config.writeEntry("SessionKey", sessionKey);
    emit validAuth();
}


void Scrobbler::nowPlaying(const FileHandle& file)
{
    KConfigGroup config(KGlobal::config(), "Scrobbling");

    //TODO: use kwallet
    QString sessionKey = config.readEntry("SessionKey", "");
    if(sessionKey.isEmpty()) {
        getAuthToken();
        return;
    }

    QMap<QString, QString> params;
    params["method"] = "track.updateNowPlaying";
    params["sk"]     = sessionKey;
    params["track"]  = file.tag()->title();
    params["artist"] = file.tag()->artist();
    params["album"]  = file.tag()->album();
    params["trackNumber"] = QString::number(file.tag()->track());
    params["duration"]    = QString::number(file.tag()->seconds());

    sign(params);
    post(params);

    m_file = file;
    m_startedPlaying = QDateTime::currentMSecsSinceEpoch() / 1000;
}

void Scrobbler::scrobble()
{
    KConfigGroup config(KGlobal::config(), "Scrobbling");

    //TODO: use kwallet
    QString sessionKey = config.readEntry("SessionKey", "");

    if(sessionKey.isEmpty()) {
        getAuthToken();
        return;
    }

    kDebug() << "Scrobbling" << m_file.tag()->title();

    QMap<QString, QString> params;
    params["method"] = "track.scrobble";
    params["sk"]     = sessionKey;
    params["track"]  = m_file.tag()->title();
    params["artist"] = m_file.tag()->artist();
    params["album"]  = m_file.tag()->album();
    params["timestamp"]   = QString::number(m_startedPlaying);
    params["trackNumber"] = QString::number(m_file.tag()->track());
    params["duration"]    = QString::number(m_file.tag()->seconds());

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
    if(data.contains("code=\"9\"")) // We need a new token
        getAuthToken();
}
