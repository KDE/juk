// musicbrainzquery.cpp
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

#include "../config.h"

#if HAVE_MUSICBRAINZ

#include "musicbrainzquery.h"

#include <kprocess.h>
#include <kdebug.h>

#include <qtimer.h>
#include <qvaluelist.h>

#include <string>
#include <vector>

MusicBrainzQuery::MusicBrainzQuery(QueryType query, QStringList args,
                                   QObject* parent, const char* name)
    : QObject(parent,name), m_query(query), m_arguments(args), m_tracks(false)
{

}

void MusicBrainzQuery::start()
{
    if (m_query == File){
#if HAVE_TRM
        KProcess *proc = new KProcess(this);
        *proc << "trm";
        *proc << m_arguments.first();
        connect(proc, SIGNAL(receivedStdout(KProcess*, char*, int)),
                SLOT(trmData(KProcess*, char*, int)));
        connect(proc, SIGNAL(processExited(KProcess *)),
                SLOT(trmGenerationFinished(KProcess *)));
        proc->start(KProcess::NotifyOnExit, KProcess::AllOutput);
#else
        emit done(TrackList());
#endif
    } else
        QTimer::singleShot(0, this, SLOT(slotQuery()));
}

void MusicBrainzQuery::slotQuery()
{
    std::string queryString;
    std::string resultString;
    std::string extractString;
    AlbumList albums;
    TrackList tracks;
    std::vector<std::string> vec;

    queryStrings(queryString, resultString, extractString);

    //UseUTF8( false );
    SetDepth( 4 );

    for(QStringList::Iterator itr = m_arguments.begin();
        itr != m_arguments.end(); ++itr) {
        vec.push_back(std::string((*itr).latin1()));
    }

    bool ret = Query(queryString, &vec);
    if( ret ) {
        std::string temp;
        int numEntries = DataInt(resultString);
        for( int i = 1; i <= numEntries; ++i ) {
            Select(extractString, i);
            if ( m_tracks ) {
                Track track(extractTrack(i));
                tracks.append(track);
            } else {
                Album alb(extractAlbum(i));
                albums.append(alb);
            }
        }
    } else {
        std::string error;
        GetQueryError(error);
        kdDebug()<<"Query failed: "<< error.c_str() <<endl;
    }

    if (m_tracks)
        emit done(tracks);
    else
        emit done(albums);

    deleteLater();//schedule deletion
}

QString MusicBrainzQuery::dataExtract(const QString& type, int i)
{
    std::string str = Data(type.latin1(), i);
    if (str.empty()) {
        return QString::null;
    } else {
        return str.c_str();
    }
}

void MusicBrainzQuery::queryStrings(std::string& query, std::string& result, std::string& extraction)
{
    switch(m_query) {
    case CD:
        query      = MBQ_GetCDInfo;
        result     = MBE_GetNumAlbums;
        extraction = MBS_SelectAlbum;
        break;
    case TrackFromTRM:
        query      = MBQ_TrackInfoFromTRMId;
        result     = MBE_GetNumTracks;
        extraction = MBS_SelectTrack;
        m_tracks   = true;
        break;
    case TrackFromID:
        query      = MBQ_QuickTrackInfoFromTrackId;
        result     = MBE_GetNumTracks;
        extraction = MBS_SelectTrack;
        m_tracks   = true;
        break;
    case ArtistByName:
        query      = MBQ_FindArtistByName;
        result     = MBE_GetNumArtists;
        extraction = MBS_SelectArtist ;
        break;
    case AlbumByName:
        query      = MBQ_FindAlbumByName;
        result     = MBE_GetNumAlbums;
        extraction = MBS_SelectAlbum ;
        break;
    case TrackByName:
        query      = MBQ_FindTrackByName;
        result     = MBE_GetNumTracks;
        extraction = MBS_SelectTrack;
        m_tracks   = true;
        break;
    case TRM:
        query      = MBQ_FindDistinctTRMId;
        result     = MBE_GetNumTrmids;
        extraction = MBS_SelectTrack;
        break;
    case ArtistByID:
        query      = MBQ_GetArtistById;
        result     = MBE_GetNumArtists;
        extraction = MBS_SelectArtist ;
        break;
    case AlbumByID:
        query      = MBQ_GetAlbumById;
        result     = MBE_GetNumAlbums;
        extraction = MBS_SelectAlbum;
        break;
    case TrackByID:
        query      = MBQ_GetTrackById;
        result     = MBE_GetNumTracks;
        extraction = MBS_SelectTrack;
        m_tracks   = true;
        break;
    default:
        kdDebug(65432)<<"Unrecognized query reported"<<endl;
    }
}

MusicBrainzQuery::Album MusicBrainzQuery::extractAlbum( int i )
{
    std::string temp;

    kdDebug()<<"Extracting "<<i<<endl;
    Album alb;
    GetIDFromURL( Data( MBE_AlbumGetAlbumId ), temp );
    alb.id = temp.c_str();
    alb.name       = dataExtract( MBE_AlbumGetAlbumName );
    alb.numTracks  = DataInt( MBE_AlbumGetNumTracks );
    alb.artist     = dataExtract( MBE_AlbumGetArtistName, 1 );
    GetIDFromURL( Data( MBE_AlbumGetArtistId, 1 ), temp );
    alb.artistId =  temp.c_str();
    alb.status     = dataExtract( MBE_AlbumGetAlbumStatus );
    alb.type       = dataExtract( MBE_AlbumGetAlbumType );
    alb.cdIndexId  = dataExtract( MBE_AlbumGetNumCdindexIds );

    TrackList tracks;
    for( int num = 1; num <= alb.numTracks; ++num  ) {
        tracks.append( extractTrackFromAlbum(num) );
    }
    alb.tracksList = tracks;

    return alb;
}

MusicBrainzQuery::Track MusicBrainzQuery::extractTrackFromAlbum(int num)
{
    Track track;
    std::string temp;
    track.num      = num;
    track.name     = dataExtract(MBE_AlbumGetTrackName, num);
    track.duration = dataExtract(MBE_AlbumGetTrackDuration, num);
    track.artist   = dataExtract(MBE_AlbumGetArtistName, num);
    GetIDFromURL( Data(MBE_AlbumGetTrackId), temp );
    track.id = ( temp.empty() )?QString::null: QString( temp.c_str() );
    GetIDFromURL( Data(MBE_AlbumGetArtistId ), temp );
    track.artistId = ( temp.empty() )? QString::null : QString( temp.c_str() );
    return track;
}

MusicBrainzQuery::Track MusicBrainzQuery::extractTrack(int num)
{
    Track track;
    std::string temp1, temp2 = Data(MBE_TrackGetTrackId);
    track.name     = dataExtract(MBE_TrackGetTrackName, num);
    track.duration = dataExtract(MBE_TrackGetTrackDuration, num);
    track.artist   = dataExtract(MBE_TrackGetArtistName, num);
    track.album    = dataExtract(MBE_AlbumGetAlbumName, num);

    GetIDFromURL( temp2, temp1 );
    track.id = ( temp1.empty() )?QString::null: QString( temp1.c_str() );
    Select(MBS_SelectTrackAlbum);
    track.num      = GetOrdinalFromList(MBE_AlbumGetTrackList, temp2);
    GetIDFromURL( Data(MBE_AlbumGetArtistId ), temp1 );
    track.artistId = ( temp1.empty() )? QString::null : QString( temp1.c_str() );
    Select(MBS_Rewind);
    return track;
}

void MusicBrainzQuery::trmData(KProcess *, char *buffer, int buflen)
{
    m_trm += QString::fromLatin1(buffer, buflen);
}

void MusicBrainzQuery::trmGenerationFinished(KProcess*)
{
    m_arguments.clear();
    m_arguments << m_trm;
    m_query = TrackFromTRM;
    kdDebug()<<"Generation finished "<<m_trm<<endl;
    slotQuery();
}

#include "musicbrainzquery.moc"

#endif
