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


#include "stringhash.h"

class Tag;
class Playlist;
class PlaylistCollection;

typedef QValueList<Playlist *> PlaylistList;

class Cache : public FileHandleHash
{
public:
    static Cache *instance();
    void save();

    static void loadPlaylists(PlaylistCollection *collection);
    static void savePlaylists(const PlaylistList &playlists);

    static bool cacheFileExists();

protected:
    Cache();
    void load();

private:
    static const int m_currentVersion = 1;
};

/**
 * A simple QDataStream subclass that has an extra field to indicate the cache
 * version.
 */

class CacheDataStream : public QDataStream
{
public:
    CacheDataStream(QIODevice *d) : QDataStream(d), m_cacheVersion(0) {}
    CacheDataStream(QByteArray a, int mode) : QDataStream(a, mode), m_cacheVersion(0) {}

    virtual ~CacheDataStream() {}

    int cacheVersion() const { return m_cacheVersion; }
    void setCacheVersion(int v) { m_cacheVersion = v; }
    
private:
    int m_cacheVersion;
};

#endif
