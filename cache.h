/***************************************************************************
                          cache.h  -  description
                             -------------------
    begin                : Sat Sep 7 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
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

#include <qstring.h>
#include <qdict.h>

#include "tag.h"

// Eventually this should implement the same interface as the Tag class; in fact
// there should be an abstract API for both of them to use.  But, for the
// moment this is just a place holder to fill in the design.

class CacheItem;

class Cache : public QDict<CacheItem>
{
public:
    class Item;

    static Cache *instance();
    CacheItem *item(const QString &fileName) const;

protected:
    Cache();
    virtual ~Cache();

private:
    static Cache *cache;
    
};

class CacheItem
{
public:
    CacheItem();
    CacheItem(const Tag &tag);
    virtual ~CacheItem();

    QString track() const { return QString::null; }
    QString artist() const { return QString::null; }
    QString album() const { return QString::null; }
    QString trackNumber() const { return QString::null; }
    QString length() const { return QString::null; }
};

#endif
