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
#include "cachedtag.h"

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
	s << it.current()->absFilePath()
	  << *(it.current());
    }

    f.writeBlock(data);
    f.close();

    QDir(dirName).rename("cache.new", "cache");

    // Store the checksum so that later we can make sure that things are ok.

    int checksum = qChecksum(data.data(), data.size());

    KConfig *config = KGlobal::config();
    {
	KConfigGroupSaver saver(config, "Cache");
	config->writeEntry("Checksum", checksum);
    }
    config->sync();
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

    QByteArray data = f.readAll();
    f.close();

    // Compare the checksum of the data to the one stored in the config file to
    // make sure that our cache isn't corrupt.

    int checksum;
    KConfig *config = KGlobal::config();
    {
	KConfigGroupSaver saver(config, "Cache");
	checksum = config->readNumEntry("Checksum", -1);
    }
    if(checksum >= 0 && checksum != qChecksum(data.data(), data.size())) {
	KMessageBox::sorry(0, i18n("The music data cache has been corrupted. JuK "
				   "needs to rescan it now. This may take some time."));
	return;
    }

    QDataStream s(data, IO_ReadOnly);

    while(!s.atEnd()) {

	QString fileName;
	s >> fileName;
	//fileName.squeeze();        

	CachedTag *t = new CachedTag(fileName);
	s >> *t;

	// Just do a dumb read from the cache.  Originally cache concistancy was
	// checked here, but this means that JuK was having to stat every file
	// in the cache while blocking GUI creation.  This has since been moved
	// to the event loop and is placed in the event loop in the 
	// CollectionListItem constructor.
    }
}
