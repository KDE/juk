/**
 * Copyright (C) 2012 Martin Sandsmark <martin.sandsmark@kde.org>
 * Copyright (C) 2014 Arnold Dumas <contact@arnolddumas.fr>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "scrobbler.h"

#include <QCryptographicHash>
#include <QDir>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>
#include <QByteArray>
#include <QUrl>
#include <QUrlQuery>

#include <kconfiggroup.h>
#include <KSharedConfig>

#include <memory>

#include "juktag.h"
#include "juk.h"
#include "juk_debug.h"

Scrobbler::Scrobbler(QObject* parent)
    : QObject(parent)
    , m_networkAccessManager(new QNetworkAccessManager(this))
    , m_wallet(Scrobbler::openKWallet())
{
    QByteArray sessionKey;

    if (m_wallet) {
        m_wallet->readEntry("SessionKey", sessionKey);
    } else {
        KConfigGroup config(KSharedConfig::openConfig(), "Scrobbling");
        sessionKey.append(config.readEntry("SessionKey", "").toLatin1());
    }

    if(sessionKey.isEmpty())
        getAuthToken();
}

bool Scrobbler::isScrobblingEnabled() // static
{
    QString username, password;

    // checks without prompting to open the wallet
    if (Wallet::folderDoesNotExist(Wallet::LocalWallet(), "JuK")) {
        KConfigGroup config(KSharedConfig::openConfig(), "Scrobbling");

        username = config.readEntry("Username", "");
        password = config.readEntry("Password", "");
    } else {
        auto wallet = Scrobbler::openKWallet();
        if (wallet) {
            QMap<QString, QString> scrobblingCredentials;
            wallet->readMap("Scrobbling", scrobblingCredentials);

            if (scrobblingCredentials.contains("Username") && scrobblingCredentials.contains("Password")) {
                username = scrobblingCredentials["Username"];
                password = scrobblingCredentials["Password"];
            }
        }
    }

    return (!username.isEmpty() && !password.isEmpty());
}

std::unique_ptr<KWallet::Wallet> Scrobbler::openKWallet() // static
{
    using KWallet::Wallet;

    const QString walletFolderName(QStringLiteral("JuK"));
    const auto walletName = Wallet::LocalWallet();

    // checks without prompting to open the wallet
    if (Wallet::folderDoesNotExist(walletName, walletFolderName)) {
        return nullptr;
    }

    std::unique_ptr<Wallet> wallet(
        Wallet::openWallet(walletName, JuK::JuKInstance()->winId()));

    if(!wallet ||
       (!wallet->hasFolder(walletFolderName) &&
           !wallet->createFolder(walletFolderName)) ||
       !wallet->setFolder(walletFolderName))
    {
        return nullptr;
    }

    return wallet;
}

QByteArray Scrobbler::md5(QByteArray data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5)
        .toHex().rightJustified(32, '0').toLower();
}

void Scrobbler::sign(QMap< QString, QString >& params)
{
    params["api_key"] = "3e6ecbd7284883089e8f2b5b53b0aecd";

    QString s;
    QMapIterator<QString, QString> i(params);

    while(i.hasNext()) {
        i.next();
        s += i.key() + i.value();
    }

    s += "2cab3957b1f70d485e9815ac1ac94096"; //shared secret

    params["api_sig"] = md5(s.toUtf8());
}

void Scrobbler::getAuthToken(QString username, QString password)
{
    qCDebug(JUK_LOG) << "Getting new auth token for user:" << username;

    QByteArray authToken = md5((username + md5(password.toUtf8())).toUtf8());

    QMap<QString, QString> params;
    params["method"]    = "auth.getMobileSession";
    params["authToken"] = authToken;
    params["username"]  = username;

    QUrl url("https://ws.audioscrobbler.com/2.0/?");

    sign(params);

    QUrlQuery urlQuery;
    const auto paramKeys = params.keys();
    for(const auto &key : paramKeys) {
        urlQuery.addQueryItem(key, params[key]);
    }

    url.setQuery(urlQuery);

    QNetworkReply *reply = m_networkAccessManager->get(QNetworkRequest(url));
    connect(reply, SIGNAL(finished()), this, SLOT(handleAuthenticationReply()));
}

void Scrobbler::getAuthToken()
{
    QString username, password;

    if (m_wallet) {

        QMap<QString, QString> scrobblingCredentials;
        m_wallet->readMap("Scrobbling", scrobblingCredentials);

        if (scrobblingCredentials.contains("Username") && scrobblingCredentials.contains("Password")) {

            username = scrobblingCredentials["Username"];
            password = scrobblingCredentials["Password"];
        }

    } else {

        KConfigGroup config(KSharedConfig::openConfig(), "Scrobbling");
        username = config.readEntry("Username", "");
        password = config.readEntry("Password", "");
    }

    if(username.isEmpty() || password.isEmpty())
        return;

    getAuthToken(username, password);
}

void Scrobbler::handleAuthenticationReply()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    qCDebug(JUK_LOG) << "got authentication reply";
    if (reply->error() != QNetworkReply::NoError) {
        emit invalidAuth();
        qCWarning(JUK_LOG) << "Error while getting authentication reply" << reply->errorString();
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
        qCWarning(JUK_LOG) << "Unable to get session key" << data;
        return;
    }

    if (m_wallet) {

        m_wallet->writeEntry("SessionKey", sessionKey.toUtf8());

    } else {

        KConfigGroup config(KSharedConfig::openConfig(), "Scrobbling");
        config.writeEntry("SessionKey", sessionKey);
    }

    emit validAuth();
}

void Scrobbler::nowPlaying(const FileHandle& file)
{
    QString sessionKey;

    if (m_wallet) {

        QByteArray sessionKeyByteArray;
        m_wallet->readEntry("SessionKey", sessionKeyByteArray);
        sessionKey = QString::fromLatin1(sessionKeyByteArray);

    } else {

        KConfigGroup config(KSharedConfig::openConfig(), "Scrobbling");
        sessionKey = config.readEntry("SessionKey", "");
    }

    if (!m_file.isNull()) {
        scrobble(); // Update time-played info for last track
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

    m_file = file; // May be empty FileHandle
    m_playbackTimer = QDateTime::currentDateTime();
}

void Scrobbler::scrobble()
{
    QString sessionKey;

    if (m_wallet) {

        QByteArray sessionKeyByteArray;
        m_wallet->readEntry("SessionKey", sessionKeyByteArray);
        sessionKey = QString::fromLatin1(sessionKeyByteArray);

    } else {

        KConfigGroup config(KSharedConfig::openConfig(), "Scrobbling");
        sessionKey = config.readEntry("SessionKey", "");
    }

    if(sessionKey.isEmpty()) {
        getAuthToken();
        return;
    }

    int halfDuration = m_file.tag()->seconds() / 2;
    int timeElapsed = m_playbackTimer.secsTo(QDateTime::currentDateTime());

    if (timeElapsed < 30 || timeElapsed < halfDuration) {
        return; // API says not to scrobble if the user didn't play long enough
    }

    qCDebug(JUK_LOG) << "Scrobbling" << m_file.tag()->title();

    QMap<QString, QString> params;
    params["method"] = "track.scrobble";
    params["sk"]     = sessionKey;
    params["track"]  = m_file.tag()->title();
    params["artist"] = m_file.tag()->artist();
    params["album"]  = m_file.tag()->album();
    params["timestamp"]   = QString::number(m_playbackTimer.toSecsSinceEpoch());
    params["trackNumber"] = QString::number(m_file.tag()->track());
    params["duration"]    = QString::number(m_file.tag()->seconds());

    sign(params);
    post(params);
}

void Scrobbler::post(QMap<QString, QString> &params)
{
    QUrl url("https://ws.audioscrobbler.com/2.0/");

    QByteArray data;
    const auto paramKeys = params.keys();
    for(const auto &key : paramKeys) {
        data += QUrl::toPercentEncoding(key) + '=' + QUrl::toPercentEncoding(params[key]) + '&';
    }

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = m_networkAccessManager->post(req, data);
    connect(reply, SIGNAL(finished()), this, SLOT(handleResults()));
}

void Scrobbler::handleResults()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray data = reply->readAll();
    if(data.contains("code=\"9\"")) // We need a new token
        getAuthToken();
}
