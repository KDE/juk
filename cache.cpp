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

#include <kstandarddirs.h>
#include <kdebug.h>

#include "cache.h"
#include "cachedtag.h"

Cache *Cache::cache = 0;

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

Cache *Cache::instance()
{
    if(cache == 0) {
	cache = new Cache();
	cache->load();
    }
    return cache;
}

void Cache::save()
{
    QString cacheFileName = KGlobal::dirs()->saveLocation("appdata") + "cache";

    QFile f(cacheFileName);

    if(!f.open(IO_WriteOnly))
	return;

    QDataStream s(&f);

    for(QDictIterator<Tag>it(*this); it.current(); ++it) {
	s << it.current()->absFilePath()
	  << *(it.current());
    }

    f.close();
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

Cache::Cache() : QDict<Tag>()
{

}

Cache::~Cache()
{
    delete cache;
}

void Cache::load()
{
    QString cacheFileName = KGlobal::dirs()->saveLocation("appdata") + "cache";

    QFile f(cacheFileName);

    if(!f.open(IO_ReadOnly))
	return;

    QDataStream s(&f);

    while(!s.atEnd()) {

	QString fileName;
	s >> fileName;

	CachedTag *t = new CachedTag(fileName);
	s >> *t;

	// Check the modification time of the file to make sure that 
	// the cache is current.

	if(!t->current())
	    delete t;
    }

    f.close();
}
