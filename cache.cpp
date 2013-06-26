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
#include "juk-exception.h"

#include <kstandarddirs.h>
#include <ksavefile.h>
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

const int Cache::playlistListCacheVersion = 3;
const int Cache::playlistItemsCacheVersion = 2;

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
    return &cache;
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
    kDebug() << "Playlists file is version" << version;

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

        // If we chose the wrong QDataStream version we may have to back out
        // and try again, so get ready by noting the playlists we have made.

        QList<Playlist *> createdPlaylists;
        bool errorOccurred = true;

        while(errorOccurred) {
            // Create a new stream just based on the data.

            QDataStream s(&data, QIODevice::ReadOnly);
            s.setVersion(dataStreamVersion);

            try { // Failure due to wrong version can be indicated by an exception

            while(!s.atEnd()) {

                qint32 playlistType;
                s >> playlistType;

                Playlist *playlist = 0;

                switch(playlistType) {
                case Search:
                {
                    SearchPlaylist *p = new SearchPlaylist(collection);
                    createdPlaylists.append(p);
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
                    createdPlaylists.append(p);
                    s >> *p;
                    playlist = p;
                    break;
                }
                default:
                    Playlist *p = new Playlist(collection, true);
                    createdPlaylists.append(p);
                    s >> *p;

                    // We may have already read this playlist from the folder
                    // scanner, if an .m3u playlist
                    if(collection->containsPlaylistFile(p->fileName())) {
                        delete p;
                        p = 0;
                    }

                    playlist = p;
                    break;
                } // switch

                if(version >= 2) {
                    qint32 sortColumn;
                    s >> sortColumn;
                    if(playlist)
                        playlist->setSorting(sortColumn);
                }

            } // while !s.atEnd()

            // Must be ok if we got this far, break out of loop.
            errorOccurred = false;

            } // try
            catch(BICStreamException &) {
                kError() << "Exception loading playlists - binary incompatible stream.";

                // Delete created playlists which probably have junk now.
                foreach(Playlist *p, createdPlaylists)
                    delete p;
                createdPlaylists.clear();

                if(dataStreamVersion == QDataStream::Qt_3_3) {
                    kError() << "Attempting other binary protocol - Qt 4.3";
                    dataStreamVersion = QDataStream::Qt_4_3;

                    break; // escape from while(!s.atEnd()) to try again
                }
#if QT_VERSION >= 0x040400
                // Unlikely, but maybe user had Qt 4.4 with KDE 4.0.0?
                else if(dataStreamVersion == QDataStream::Qt_4_3) {
                    kError() << "Attempting other binary protocol - Qt 4.4";
                    dataStreamVersion = QDataStream::Qt_4_4;

                    break;
                }
#endif
                // We tried 3.3 first, if 4.3/4.4 doesn't work who knows...
                kError() << "Unable to recover, no playlists will be loaded.";
                return;
            } // catch
        } // while dataStreamVersion != -1
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
    QString playlistsFile = dirName + "playlists";
    KSaveFile f(playlistsFile);

    if(!f.open(QIODevice::WriteOnly)) {
        kError() << "Error saving collection:" << f.errorString();
        return;
    }

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
    fs << qint32(playlistListCacheVersion);
    fs << qChecksum(data.data(), data.size());

    fs << data;
    f.close();

    if(!f.finalize())
        kError() << "Error saving collection:" << f.errorString();
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

bool Cache::prepareToLoadCachedItems()
{
    QString cacheFileName = KGlobal::dirs()->saveLocation("appdata") + "cache";

    m_loadFile.setFileName(cacheFileName);
    if(!m_loadFile.open(QIODevice::ReadOnly))
        return false;

    m_loadDataStream.setDevice(&m_loadFile);

    int dataStreamVersion = CacheDataStream::Qt_3_3;

    qint32 version;
    m_loadDataStream >> version;

    switch(version) {
    case 2:
        dataStreamVersion = CacheDataStream::Qt_4_3;

        // Other than that we're compatible with cache v1, so fallthrough
        // to setCacheVersion

    case 1: {
        m_loadDataStream.setCacheVersion(1);
        m_loadDataStream.setVersion(dataStreamVersion);

        qint32 checksum;
        m_loadDataStream >> checksum
          >> m_loadFileBuffer.buffer();

        m_loadFileBuffer.open(QIODevice::ReadOnly);
        m_loadDataStream.setDevice(&m_loadFileBuffer);

        qint32 checksumExpected = qChecksum(
                m_loadFileBuffer.data(), m_loadFileBuffer.size());
        if(m_loadDataStream.status() != CacheDataStream::Ok ||
                checksum != checksumExpected)
        {
            kError() << "Music cache checksum expected to get" << checksumExpected <<
                        "actually was" << checksum;
            KMessageBox::sorry(0, i18n("The music data cache has been corrupted. JuK "
                                       "needs to rescan it now. This may take some time."));
            return false;
        }

        break;
    }
    default: {
        m_loadDataStream.device()->reset();
        m_loadDataStream.setCacheVersion(0);

        // This cache is so old that this is just a wild guess here that 3.3
        // is compatible.
        m_loadDataStream.setVersion(CacheDataStream::Qt_3_3);
        break;
    }
    }

    return true;
}

FileHandle Cache::loadNextCachedItem()
{
    if(!m_loadFile.isOpen() || !m_loadDataStream.device()) {
        kWarning() << "Already completed reading cache file.";
        return FileHandle::null();
    }

    if(m_loadDataStream.status() == QDataStream::ReadCorruptData) {
        kError() << "Attempted to read file handle from corrupt cache file.";
        return FileHandle::null();
    }

    if(!m_loadDataStream.atEnd()) {
        QString fileName;
        m_loadDataStream >> fileName;
        fileName.squeeze();

        return FileHandle(fileName, m_loadDataStream);
    }
    else {
        m_loadDataStream.setDevice(0);
        m_loadFile.close();

        return FileHandle::null();
    }
}

// vim: set et sw=4 tw=0 sta:
