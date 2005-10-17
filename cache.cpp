/***************************************************************************
    begin                : Sat Sep 7 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kactionclasses.h>
#include <kdebug.h>

#include <qdir.h>
#include <qbuffer.h>

#include "cache.h"
#include "tag.h"
#include "searchplaylist.h"
#include "historyplaylist.h"
#include "upcomingplaylist.h"
#include "folderplaylist.h"
#include "playlistcollection.h"
#include "actioncollection.h"

using namespace ActionCollection;

static const int playlistCacheVersion = 2;

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

    if(!f.open(IO_WriteOnly))
        return;

    QByteArray data;
    QDataStream s(data, IO_WriteOnly);

    for(Iterator it = begin(); it != end(); ++it) {
        s << (*it).absFilePath();
        s << *it;
    }

    QDataStream fs(&f);

    Q_INT32 checksum = qChecksum(data.data(), data.size());

    fs << Q_INT32(m_currentVersion)
       << checksum
       << data;

    f.close();

    QDir(dirName).rename("cache.new", "cache");
}

void Cache::loadPlaylists(PlaylistCollection *collection) // static
{
    QString playlistsFile = KGlobal::dirs()->saveLocation("appdata") + "playlists";

    QFile f(playlistsFile);

    if(!f.open(IO_ReadOnly))
        return;

    QDataStream fs(&f);

    Q_INT32 version;
    fs >> version;

    switch(version) {
    case 1:
    case 2:
    {
        // Our checksum is only for the values after the version and checksum so
        // we want to get a byte array with just the checksummed data.

        QByteArray data;
        Q_UINT16 checksum;
        fs >> checksum >> data;

        if(checksum != qChecksum(data.data(), data.size()))
            return;

        // Create a new stream just based on the data.

        QDataStream s(data, IO_ReadOnly);

        while(!s.atEnd()) {

            Q_INT32 playlistType;
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
            if(version == 2) {
                Q_INT32 sortColumn;
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

    if(!f.open(IO_WriteOnly))
        return;

    QByteArray data;
    QDataStream s(data, IO_WriteOnly);

    for(PlaylistList::ConstIterator it = playlists.begin(); it != playlists.end(); ++it) {
        if(*it) {
            if(dynamic_cast<HistoryPlaylist *>(*it)) {
                s << Q_INT32(History)
                  << *static_cast<HistoryPlaylist *>(*it);
            }
            else if(dynamic_cast<SearchPlaylist *>(*it)) {
                s << Q_INT32(Search)
                  << *static_cast<SearchPlaylist *>(*it);
            }
            else if(dynamic_cast<UpcomingPlaylist *>(*it)) {
                if(!action<KToggleAction>("saveUpcomingTracks")->isChecked())
                    continue;
                s << Q_INT32(Upcoming)
                  << *static_cast<UpcomingPlaylist *>(*it);
            }
            else if(dynamic_cast<FolderPlaylist *>(*it)) {
                s << Q_INT32(Folder)
                  << *static_cast<FolderPlaylist *>(*it);
            }
            else {
                s << Q_INT32(Normal)
                  << *(*it);
            }
            s << Q_INT32((*it)->sortColumn());
        }
    }

    QDataStream fs(&f);
    fs << Q_INT32(playlistCacheVersion);
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

    if(!f.open(IO_ReadOnly))
        return;

    CacheDataStream s(&f);

    Q_INT32 version;
    s >> version;

    QBuffer buffer;

    // Do the version specific stuff.

    switch(version) {
    case 1: {
        s.setCacheVersion(1);

        Q_INT32 checksum;
        QByteArray data;
        s >> checksum
          >> data;

        buffer.setBuffer(data);
        buffer.open(IO_ReadOnly);
        s.setDevice(&buffer);

        if(checksum != qChecksum(data.data(), data.size())) {
            KMessageBox::sorry(0, i18n("The music data cache has been corrupted. JuK "
                                       "needs to rescan it now. This may take some time."));
            return;
        }
        break;
    }
    default: {
        s.device()->reset();
        s.setCacheVersion(0);
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
