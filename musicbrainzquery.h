// -*- Mode: c++-mode; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// musicbrainzquery.h
//
// Copyright (C)  2003  Zack Rusin <zack@kde.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.

#ifndef MUSICBRAINZ_H
#define MUSICBRAINZ_H

#include "../config.h"

#if HAVE_MUSICBRAINZ

#include <musicbrainz/musicbrainz.h>
#include <qobject.h>
#include <qstringlist.h>

class KProcess;

/**
 * This is a class used to issue MusicBrainz queries.
 * It's pseudo-asynchrnous. Pseudo because it depends on
 * asynchrnous libmusicbrainz (someday we'll have to write
 * our own KDE native one :) ). The type of queries are in the
 * Query enum. You have to specify the query with arguments in
 * the constructor. Connect to the query signals and issue the
 * start() call on the job. So for example to find an album by
 * name one would do :
 *
 * QStringList l;
 * l<<"h2o";
 * MusicBrainzQuery *query =
 *       new MusicBrainzQuery( MusicBrainzQuery::AlbumByName ,
 *                          l );
 * connect( query, SIGNAL(done(const MusicBrainzQuery::AlbumList&)),
 *         SLOT(slotDone(const MusicBrainzQuery::AlbumList&)) );
 * query->start();
 *
 */

class MusicBrainzQuery : public QObject,
                         public MusicBrainz
{
    Q_OBJECT
public:
    enum QueryType {
        CD,//!identifies the cd , doesn't take any arguments
        File,//!tries to identify the given file, takes file path
        TrackFromTRM,//!identifies the song from trm, takes the trm
        TrackFromID,//!song from track id, takes the trackId
        ArtistByName,//!name
        AlbumByName,//!name
        TrackByName,//!name
        TRM,//!artist name + track name
        ArtistByID,//!artist id
        AlbumByID,//!album id
        TrackByID//!track id
    };
    struct Track {
        int     num;
        QString id;
        QString album;
        QString name;
        QString duration;
        QString artist;
        QString artistId;
    };
    typedef QValueList<Track> TrackList;
    struct Album {
        Album(): numTracks(0) {}
        QString name;
        QString artist;
        QString id;
        QString status;
        QString type;
        QString cdIndexId;
        QString artistId;
        int     numTracks;
        TrackList tracksList;
    };
    typedef QValueList<Album> AlbumList;

    MusicBrainzQuery(QueryType query, const QStringList& args,
                     QObject* parent=0, const char* name=0);

    void start();

signals:
    void done(const MusicBrainzQuery::AlbumList&);
    void done(const MusicBrainzQuery::TrackList&);

protected slots:
    void slotQuery();
    void trmData(KProcess *proc, char *buffer, int buflen);
    void trmGenerationFinished(KProcess *proc);

protected:
    void        trmQuery(const QString& file);
    QString     dataExtract(const QString&, int i=0);
    void        queryStrings(std::string& query, std::string& result, std::string& extraction);
    Album       extractAlbum(int);
    Track       extractTrack(int);
    Track       extractTrackFromAlbum(int);

    QueryType   m_query;
    QStringList m_arguments;
    QString     m_trm;
    bool        m_tracks;//if only tracks should be extracted
};

#endif

#endif
