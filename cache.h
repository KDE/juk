/***************************************************************************
                          cache.h  -  description
                             -------------------
    begin                : Sat Sep 7 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#include <qdict.h>

class Tag;

class Cache : public QDict<Tag>
{
public:
    static Cache *instance();
    void save();

protected:
    Cache();
    void load();

private:
    static Cache *m_cache;
    /**
     * Note this number is a prime number that should be larger than the target
     * size of the dict.
     */
    static const int m_cacheSize = 5003;
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
