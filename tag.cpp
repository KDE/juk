/***************************************************************************
                          tag.cpp  -  description
                             -------------------
    begin                : Sun Feb 17 2002
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

#include <kdebug.h>

#include <qregexp.h>

#include "tag.h"
#include "id3tag.h"
#include "oggtag.h"
#include "cachedtag.h"
#include "cache.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Tag *Tag::createTag(const QString &file, bool ignoreCache)
{
    Tag *cachedItem = 0;

    if(!ignoreCache)
        cachedItem = Cache::instance()->find(file);

    if(cachedItem)
        return cachedItem;

    if(file.lower().endsWith("mp3"))
        return new ID3Tag(file);

    if(file.lower().endsWith("ogg"))
        return new OggTag(file);

    return 0;
}

Tag::~Tag()
{
    Cache::instance()->remove(absFilePath());
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

Tag::Tag(const QString &file) : m_info(file), m_fileName(file)
{
    Cache::instance()->insert(file, this);
}

QString Tag::readBitrate(const KFileMetaInfo &metaInfo)
{
    if(metaInfo.isValid() && !metaInfo.isEmpty())
	return metaInfo.item("Bitrate").string().stripWhiteSpace().section(' ', 0, 0);
    else
	return QString::null;
}

QString Tag::readLength(const KFileMetaInfo &metaInfo)
{
    if(metaInfo.isValid() && !metaInfo.isEmpty())
	return metaInfo.item("Length").string().stripWhiteSpace().replace(QRegExp("^0+"), QString::null);
    else
	return QString::null;
}

int Tag::readSeconds(const KFileMetaInfo &metaInfo)
{
    QStringList l = QStringList::split(':', readLength(metaInfo));

    int total = 0;

    for(QStringList::Iterator it = l.begin(); it != l.end(); ++it)
	total = 60 * total + (*it).toInt();
    
    return total;
}

////////////////////////////////////////////////////////////////////////////////
// related functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const Tag &t)
{
    s << t.hasTag()

      << t.track()
      << t.artist()
      << t.album()
      << t.genre()
      << t.trackNumber()
      << t.trackNumberString()
      << t.year()
      << t.yearString()
      << t.comment()

      << t.bitrateString()
      << t.lengthString()
      << t.seconds()

      << t.absFilePath()
      << t.lastModified();

    return s;
}
