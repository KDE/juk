/***************************************************************************
                          cache.cpp  -  description
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

#include "cache.h"

Cache *Cache::cache = 0;

////////////////////////////////////////////////////////////////////////////////
// Cache public methods
////////////////////////////////////////////////////////////////////////////////

Cache *Cache::instance()
{
    if(!cache)
	cache = new Cache();
    return(cache);
}

CacheItem *Cache::item(const QString &file) const
{
    return(0);
}

////////////////////////////////////////////////////////////////////////////////
// Cache protected methods
////////////////////////////////////////////////////////////////////////////////

Cache::Cache()
{
    setAutoDelete(true);
}

Cache::~Cache()
{
    delete(cache);
}

////////////////////////////////////////////////////////////////////////////////
// CacheItem public methods
////////////////////////////////////////////////////////////////////////////////

CacheItem::CacheItem()
{

}

CacheItem::CacheItem(const Tag &tag)
{

}

CacheItem::~CacheItem()
{

}
