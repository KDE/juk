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
#if 0 /// TODO: This should be included the next time that the cache format changes
#include <kdatastream.h>
#endif
#include <qregexp.h>

#include "tag.h"
#include "id3tag.h"
#include "oggtag.h"
#include "cache.h"
#include "mediafiles.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Tag *Tag::createTag(const QString &fileName, bool ignoreCache)
{
    Tag *cachedItem = 0;

    if(!ignoreCache)
        cachedItem = Cache::instance()->find(fileName);

    if(cachedItem)
        return cachedItem;

    if(MediaFiles::isMP3(fileName))
        return new ID3Tag(fileName);

    if(MediaFiles::isOgg(fileName))
        return new OggTag(fileName);

    if(MediaFiles::isFLAC(fileName))
        return new OggTag(fileName);

    kdError() << "Couldn't resolve the mime type of \"" << fileName << "\" -- this shouldn't happen." << endl;

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
	return metaInfo.item("Length").string().stripWhiteSpace().replace(QRegExp("^0+([0-9])"), "\\1");
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
      /// TODO: Use Q_UINT32 in place of all integers.
#if 0 /// TODO: This should be included the next time that the cache format changes
    s << t.hasTag()
#else
    s << int(t.hasTag())
#endif
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
