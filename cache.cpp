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
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdebug.h>

#include <qdir.h>
#include <qbuffer.h>

#include "cache.h"
#include "tag.h"

Cache *Cache::m_cache = 0;

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

Cache *Cache::instance()
{
    if(m_cache == 0) {
	m_cache = new Cache;
	m_cache->load();
    }
    return m_cache;
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

    for(QDictIterator<Tag>it(*this); it.current(); ++it) {
	s << it.current()->fileName();
	s << *(it.current());
    }

    QDataStream fs(&f);

    Q_INT32 checksum = qChecksum(data.data(), data.size());

    fs << Q_INT32(m_currentVersion)
       << checksum
       << data;

    f.close();

    QDir(dirName).rename("cache.new", "cache");
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

Cache::Cache() : QDict<Tag>(m_cacheSize)
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

	Tag *t = new Tag(fileName);
	s >> *t;
    }
}
