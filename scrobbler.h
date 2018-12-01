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

#ifndef JUK_SCROBBLER_H
#define JUK_SCROBBLER_H

#include <QObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QDateTime>

#include <KWallet>

#include <memory>

#include "filehandle.h"

using namespace KWallet;

class QByteArray;
class QNetworkAccessManager;

/**
 * A class that handles scrobbling of tracks to last.fm
 */
class Scrobbler : public QObject {
    Q_OBJECT

public:
    explicit Scrobbler(QObject* parent = nullptr);

    static bool isScrobblingEnabled();
    static std::unique_ptr<Wallet> openKWallet();

public slots:
    void nowPlaying(const FileHandle&);
    void scrobble();
    void getAuthToken(QString username, QString password);

private slots:
    void handleAuthenticationReply();
    void handleResults();
    void getAuthToken();

signals:
    void invalidAuth();
    void validAuth();

private:
    void sign(QMap<QString, QString> &request);
    void post(QMap<QString, QString> &request);
    QByteArray md5(QByteArray data);

    QDateTime m_playbackTimer;
    FileHandle m_file;

    std::unique_ptr<QNetworkAccessManager> m_networkAccessManager;
    std::unique_ptr<Wallet> m_wallet;
};

#endif /* JUK_SCROBBLER_H */
