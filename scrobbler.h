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

#ifndef JUK_SCROBBLER_H
#define JUK_SCROBBLER_H

#include <QObject>
#include <QMap>
#include <QDateTime>

#include "filehandle.h"

class QByteArray;
class QNetworkAccessManager;

/**
 * A class that handles scrobbling of tracks to last.fm
 */
class Scrobbler : public QObject {
    Q_OBJECT
public:
    explicit Scrobbler(QObject* parent = 0);
    virtual ~Scrobbler();

    static bool isScrobblingEnabled();

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
    QNetworkAccessManager *m_networkAccessManager;
};

#endif /* JUK_SCROBBLER_H */
