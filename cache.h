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

#ifndef JUK_CACHE_H
#define JUK_CACHE_H

#include <QDataStream>
#include <QFile>
#include <QBuffer>

class Playlist;
class PlaylistCollection;
class FileHandle;

template<class T>
class QVector;

typedef QVector<Playlist *> PlaylistList;

/**
 * A simple QDataStream subclass that has an extra field to indicate the cache
 * version.
 */

class CacheDataStream : public QDataStream
{
public:
    CacheDataStream(QIODevice *d) : QDataStream(d), m_cacheVersion(0) {}
    CacheDataStream() : m_cacheVersion(0) { }

    int cacheVersion() const { return m_cacheVersion; }
    void setCacheVersion(int v) { m_cacheVersion = v; }

private:
    int m_cacheVersion;
};


class Cache
{
public:
    static Cache *instance();

    static void loadPlaylists(PlaylistCollection *collection);
    static void savePlaylists(const PlaylistList &playlists);

    static void ensureAppDataStorageExists();
    static bool cacheFileExists();

    static QString fileHandleCacheFileName();
    static QString playlistsCacheFileName();

    bool prepareToLoadCachedItems();
    FileHandle loadNextCachedItem();

    /**
     * QDataStream version for serialized list of playlists
     * 1, 2: Who knows?
     * 3: Current.
     */
    static const int playlistListCacheVersion;

    /**
     * QDataStream version for serialized list of playlist items in a playlist
     * 1: Original cache version
     * 2: KDE 4.0.1+, explicitly sets QDataStream encoding.
     */
    static const int playlistItemsCacheVersion;

private:
    // private to force access through instance()
    Cache();

private:
    QFile m_loadFile;
    QBuffer m_loadFileBuffer;
    CacheDataStream m_loadDataStream;
};

#endif

// vim: set et sw=4 tw=0 sta:
