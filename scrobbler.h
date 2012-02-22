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

#ifndef SCROBBLER_H
#define SCROBBLER_H

#include <QObject>
#include <QByteArray>
#include <QMap>

#include "filehandle.h"

/**
 * A class that handles scrobbling of tracks to last.fm
 */

class Scrobbler : public QObject {
    Q_OBJECT
public:
    explicit Scrobbler(QObject* parent = 0);
    virtual ~Scrobbler();
    
public slots:
    void nowPlaying(const FileHandle&);
    void scrobble();
    
private slots:
    void getAuthToken();
    void handleAuthenticationReply();
    void handleResults();
    
    
private:
    void sign(QMap<QString, QString> &request);
    void post(QMap<QString, QString> &request);
    QByteArray md5(QByteArray data);
    
    FileHandle m_file;
    qint64 m_startedPlaying;
    QString m_sessionKey;
};

#endif /* SCROBBLER_H */
