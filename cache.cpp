/***************************************************************************
    begin                : Sat Sep 7 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2008 by Michael Pyne
    email                : michael.pyne@kdemail.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "cache.h"

#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>
#include <ktoggleaction.h>

#include <QDir>
#include <QBuffer>

#include "tag.h"
#include "searchplaylist.h"
#include "historyplaylist.h"
#include "upcomingplaylist.h"
#include "folderplaylist.h"
#include "playlistcollection.h"
#include "actioncollection.h"

using namespace ActionCollection;

static const int playlistCacheVersion = 3;

enum PlaylistType
{
    Normal   = 0,
    Search   = 1,
    History  = 2,
    Upcoming = 3,
    Folder   = 4
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

Cache *Cache::instance()
{
    static Cache cache;

    // load() indirectly calls instance() so we have to protect against recursion.
    static bool loaded = false;

    if(!loaded) {
        loaded = true;
        cache.load();
    }

    return &cache;
}

void Cache::save()
{
    QString dirName = KGlobal::dirs()->saveLocation("appdata");
    QString cacheFileName =  dirName + "cache.new";

    QFile f(cacheFileName);

    // TODO: Investigate KSaveFile

    if(!f.open(QIODevice::WriteOnly))
        return;

    QByteArray data;
    QDataStream s(&data, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_4_3);

    for(Iterator it = begin(); it != end(); ++it) {
        s << (*it).absFilePath();
        s << *it;
    }

    QDataStream fs(&f);

    qint32 checksum = qChecksum(data.data(), data.size());

    fs << qint32(m_currentVersion)
       << checksum
       << data;

    f.close();

    QDir dir(dirName);
    dir.remove("cache");
    dir.rename("cache.new", "cache");
}

void Cache::loadPlaylists(PlaylistCollection *collection) // static
{
    QString playlistsFile = KGlobal::dirs()->saveLocation("appdata") + "playlists";

    QFile f(playlistsFile);

    if(!f.open(QIODevice::ReadOnly))
        return;

    QDataStream fs(&f);
    int dataStreamVersion = QDataStream::Qt_3_3;

    qint32 version;
    fs >> version;

    switch(version) {
    case 3:
        dataStreamVersion = QDataStream::Qt_4_3;
        // Fall-through

    case 1:
    case 2:
    {
        // Our checksum is only for the values after the version and checksum so
        // we want to get a byte array with just the checksummed data.

        QByteArray data;
        quint16 checksum;
        fs >> checksum >> data;

        if(checksum != qChecksum(data.data(), data.size()))
            return;

        // Create a new stream just based on the data.

        QDataStream s(&data, QIODevice::ReadOnly);
        s.setVersion(dataStreamVersion);

        while(!s.atEnd()) {

            qint32 playlistType;
            s >> playlistType;

            Playlist *playlist = 0;

            switch(playlistType) {
            case Search:
            {
                SearchPlaylist *p = new SearchPlaylist(collection);
                s >> *p;
                playlist = p;
                break;
            }
            case History:
            {
                action<KToggleAction>("showHistory")->setChecked(true);
                collection->setHistoryPlaylistEnabled(true);
                s >> *collection->historyPlaylist();
                playlist = collection->historyPlaylist();
                break;
            }
            case Upcoming:
            {
                /*
                collection->setUpcomingPlaylistEnabled(true);
                Playlist *p = collection->upcomingPlaylist();
                action<KToggleAction>("saveUpcomingTracks")->setChecked(true);
                s >> *p;
                playlist = p;
                */
                break;
            }
            case Folder:
            {
                FolderPlaylist *p = new FolderPlaylist(collection);
                s >> *p;
                playlist = p;
                break;
            }
            default:
                Playlist *p = new Playlist(collection, true);
                s >> *p;
                playlist = p;
                break;
            }

            if(version >= 2) {
                qint32 sortColumn;
                s >> sortColumn;
                if(playlist)
                    playlist->setSorting(sortColumn);
            }
        }
        break;
    }
    default:
    {
        // Because the original version of the playlist cache did not contain a
        // version number, we want to revert to the beginning of the file before
        // reading the data.

        f.reset();

         while(!fs.atEnd()) {
            Playlist *p = new Playlist(collection);
            fs >> *p;
        }
        break;
    }
    }

    f.close();
}

void Cache::savePlaylists(const PlaylistList &playlists)
{
    QString dirName = KGlobal::dirs()->saveLocation("appdata");
    QString playlistsFile = dirName + "playlists.new";
    QFile f(playlistsFile);

    // TODO: Investigate KSaveFile

    if(!f.open(QIODevice::WriteOnly))
        return;

    QByteArray data;
    QDataStream s(&data, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_4_3);

    for(PlaylistList::ConstIterator it = playlists.begin(); it != playlists.end(); ++it) {
        if(*it) {
            if(dynamic_cast<HistoryPlaylist *>(*it)) {
                s << qint32(History)
                  << *static_cast<HistoryPlaylist *>(*it);
            }
            else if(dynamic_cast<SearchPlaylist *>(*it)) {
                s << qint32(Search)
                  << *static_cast<SearchPlaylist *>(*it);
            }
            else if(dynamic_cast<UpcomingPlaylist *>(*it)) {
                if(!action<KToggleAction>("saveUpcomingTracks")->isChecked())
                    continue;
                s << qint32(Upcoming)
                  << *static_cast<UpcomingPlaylist *>(*it);
            }
            else if(dynamic_cast<FolderPlaylist *>(*it)) {
                s << qint32(Folder)
                  << *static_cast<FolderPlaylist *>(*it);
            }
            else {
                s << qint32(Normal)
                  << *(*it);
            }
            s << qint32((*it)->sortColumn());
        }
    }

    QDataStream fs(&f);
    fs << qint32(playlistCacheVersion);
    fs << qChecksum(data.data(), data.size());

    fs << data;
    f.close();

    QDir(dirName).rename("playlists.new", "playlists");
}

bool Cache::cacheFileExists() // static
{
    return QFile::exists(KGlobal::dirs()->saveLocation("appdata") + "cache");
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

Cache::Cache() : FileHandleHash()
{

}

void Cache::load()
{
    QString cacheFileName = KGlobal::dirs()->saveLocation("appdata") + "cache";

    QFile f(cacheFileName);

    if(!f.open(QIODevice::ReadOnly))
        return;

    CacheDataStream s(&f);
    int dataStreamVersion = CacheDataStream::Qt_3_3;

    qint32 version;
    s >> version;

    QBuffer buffer;
    QByteArray data;

    // Do the version specific stuff.

    switch(version) {
    case 2:
        dataStreamVersion = CacheDataStream::Qt_4_3;

        // Other than that we're compatible with cache v1, so fallthrough
        // to setCacheVersion

    case 1: {
        s.setCacheVersion(1);
        s.setVersion(dataStreamVersion);

        qint32 checksum;
        s >> checksum
          >> data;

        buffer.setBuffer(&data);
        buffer.open(QIODevice::ReadOnly);
        s.setDevice(&buffer);

        if(s.status() != CacheDataStream::Ok ||
            checksum != qChecksum(data.data(), data.size()))
        {
            KMessageBox::sorry(0, i18n("The music data cache has been corrupted. JuK "
                                       "needs to rescan it now. This may take some time."));
            return;
        }

        break;
    }
    default: {
        s.device()->reset();
        s.setCacheVersion(0);

        // This cache is so old that this is just a wild guess here that 3.3
        // is compatible.
        s.setVersion(CacheDataStream::Qt_3_3);
        break;
    }
    }

    // Read the cached tags.

    while(!s.atEnd()) {
        QString fileName;
        s >> fileName;
        fileName.squeeze();

        FileHandle f(fileName, s);
    }
}

// vim: set et sw=4 tw=0 sta:
