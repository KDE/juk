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

#ifndef CACHE_H
#define CACHE_H

#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QBuffer>

#include "stringhash.h"

class Playlist;
class PlaylistCollection;

template<class T>
class QList;

typedef QList<Playlist *> PlaylistList;

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


class Cache : public FileHandleHash
{
public:
    static Cache *instance();

    static void loadPlaylists(PlaylistCollection *collection);
    static void savePlaylists(const PlaylistList &playlists);

    static bool cacheFileExists();

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

protected:
    Cache();

private:
    QFile m_loadFile;
    QBuffer m_loadFileBuffer;
    CacheDataStream m_loadDataStream;
};

#endif

// vim: set et sw=4 tw=0 sta:
