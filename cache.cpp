/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2008, 2013 Michael Pyne <mpyne@kde.org>
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

#include "cache.h"
#include "juk-exception.h"

#include <kmessagebox.h>
#include <kconfig.h>
#include <ktoggleaction.h>
#include <KLocalizedString>

#include <QDir>
#include <QBuffer>
#include <QtGlobal>
#include <QSaveFile>

#include "juktag.h"
#include "searchplaylist.h"
#include "historyplaylist.h"
#include "upcomingplaylist.h"
#include "folderplaylist.h"
#include "playlistcollection.h"
#include "actioncollection.h"
#include "juk.h"
#include "juk_debug.h"

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

static void parsePlaylistStream(QDataStream &s, PlaylistCollection *collection)
{
    while(!s.atEnd()) {
        qint32 playlistType;
        s >> playlistType;

        Playlist *playlist = nullptr;

        switch(playlistType) {
        case Search:
        {
            SearchPlaylist *p = new SearchPlaylist(collection, *(new PlaylistSearch(JuK::JuKInstance())));
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

            // We may have already read this playlist from the folder
            // scanner, if an .m3u playlist
            if(collection->containsPlaylistFile(p->fileName())) {
                delete p;
                p = nullptr;
            }

            playlist = p;
            break;
        } // switch

        qint32 sortColumn;
        s >> sortColumn;
        if(playlist)
            playlist->sortByColumn(sortColumn);
    }
}

void Cache::loadPlaylists(PlaylistCollection *collection) // static
{
    const QString playlistsFile = playlistsCacheFileName();
    QFile f(playlistsFile);

    if(!f.open(QIODevice::ReadOnly))
        return;

    QDataStream fs(&f);

    qint32 version;
    fs >> version;

    if(version != 3 || fs.status() != QDataStream::Ok) {
        // Either the file is corrupt or is from a truly ancient version
        // of JuK.
        qCWarning(JUK_LOG) << "Found the playlist cache but it was clearly corrupt.";
        return;
    }

    // Our checksum is only for the values after the version and checksum so
    // we want to get a byte array with just the checksummed data.

    QByteArray data;
    quint16 checksum;
    fs >> checksum >> data;

    if(fs.status() != QDataStream::Ok || checksum != qChecksum(data.data(), data.size()))
        return;

    QDataStream s(&data, QIODevice::ReadOnly);
    s.setVersion(QDataStream::Qt_4_3);

    try { // Loading failures are indicated by an exception
        parsePlaylistStream(s, collection);
    }
    catch(BICStreamException &) {
        qCCritical(JUK_LOG) << "Exception loading playlists - binary incompatible stream.";
        // TODO Restructure the Playlist data model and PlaylistCollection data model
        // to be separate from the view/controllers.
        return;
    }
}

void Cache::savePlaylists(const PlaylistList &playlists)
{
    QString playlistsFile = playlistsCacheFileName();
    QSaveFile f(playlistsFile);

    if(!f.open(QIODevice::WriteOnly)) {
        qCCritical(JUK_LOG) << "Error saving collection:" << f.errorString();
        return;
    }

    QByteArray data;
    QDataStream s(&data, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_4_3);

    for(const auto &it : playlists) {
        if(!(it)) {
            continue;
        }
        // TODO back serialization type into Playlist itself
        if(dynamic_cast<HistoryPlaylist *>(it)) {
            s << qint32(History)
                << *static_cast<HistoryPlaylist *>(it);
        }
        else if(dynamic_cast<SearchPlaylist *>(it)) {
            s << qint32(Search)
                << *static_cast<SearchPlaylist *>(it);
        }
        else if(dynamic_cast<UpcomingPlaylist *>(it)) {
            if(!action<KToggleAction>("saveUpcomingTracks")->isChecked())
                continue;
            s << qint32(Upcoming)
                << *static_cast<UpcomingPlaylist *>(it);
        }
        else if(dynamic_cast<FolderPlaylist *>(it)) {
            s << qint32(Folder)
                << *static_cast<FolderPlaylist *>(it);
        }
        else {
            s << qint32(Normal)
                << *(it);
        }
        s << qint32(it->sortColumn());
    }

    QDataStream fs(&f);
    fs << qint32(playlistListCacheVersion);
    fs << qChecksum(data.data(), data.size());

    fs << data;

    if(!f.commit())
        qCCritical(JUK_LOG) << "Error saving collection:" << f.errorString();
}

void Cache::ensureAppDataStorageExists() // static
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir appDataDir(dirPath);

    if(!appDataDir.exists() && !appDataDir.mkpath(dirPath))
        qCCritical(JUK_LOG) << "Unable to create appdata storage in" << dirPath;
}

bool Cache::cacheFileExists() // static
{
    return QFile::exists(fileHandleCacheFileName());
}

// Despite the 'Cache' class name, these data files are not regenerable and so
// should not be stored in cache directory.
QString Cache::fileHandleCacheFileName() // static
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/cache";
}

QString Cache::playlistsCacheFileName() // static
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/playlists";
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

Cache::Cache()
{

}

bool Cache::prepareToLoadCachedItems()
{
    m_loadFile.setFileName(fileHandleCacheFileName());
    if(!m_loadFile.open(QIODevice::ReadOnly))
        return false;

    m_loadDataStream.setDevice(&m_loadFile);

    int dataStreamVersion = CacheDataStream::Qt_3_3;

    qint32 version;
    m_loadDataStream >> version;

    switch(version) {
    case 2:
        dataStreamVersion = CacheDataStream::Qt_4_3;
        Q_FALLTHROUGH();

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
            qCCritical(JUK_LOG) << "Music cache checksum expected to get" << checksumExpected <<
                        "actually was" << checksum;
            KMessageBox::error(0, i18n("The music data cache has been corrupted. JuK "
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
        qCWarning(JUK_LOG) << "Already completed reading cache file.";
        return FileHandle();
    }

    if(m_loadDataStream.status() == QDataStream::ReadCorruptData) {
        qCCritical(JUK_LOG) << "Attempted to read file handle from corrupt cache file.";
        return FileHandle();
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

        return FileHandle();
    }
}

// vim: set et sw=4 tw=0 sta:
