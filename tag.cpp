/***************************************************************************
                          tag.cpp  -  description
                             -------------------
    begin                : Sun Feb 17 2002
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

#include <qregexp.h>

#include "tag.h"
#include "id3tag.h"
#include "oggtag.h"
#include "cachedtag.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Tag *Tag::createTag(const QString &file)
{
    QFileInfo f(file);
    QString extension = f.extension(false).lower();
    
    // insert a check for a cache hit here

    if(extension == "mp3")
	return new ID3Tag(file);
    if(extension == "ogg")
	return new OggTag(file);
    else
	return(0);
}

Tag::~Tag()
{

}

QString Tag::absFilePath() const
{
    return(fileInfo.absFilePath());
}

QDateTime Tag::lastModified() const
{
    return(fileInfo.lastModified());
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

Tag::Tag(const QString &file)
{
    fileInfo.setFile(file);
}

QString Tag::readBitrate(const KFileMetaInfo &metaInfo)
{
    if(metaInfo.isValid() && !metaInfo.isEmpty())
	return(metaInfo.item("Bitrate").string().stripWhiteSpace().section(' ', 0, 0));
    else
	return(QString::null);
}

QString Tag::readLength(const KFileMetaInfo &metaInfo)
{
    if(metaInfo.isValid() && !metaInfo.isEmpty())
	return(metaInfo.item("Length").string().stripWhiteSpace().remove(QRegExp("^0+")));
    else
	return(QString::null);
}

int Tag::readSeconds(const KFileMetaInfo &metaInfo)
{
    QStringList l = QStringList::split(':', readLength(metaInfo));

    int total = 0;

    for(QStringList::Iterator it = l.begin(); it != l.end(); ++it)
	total = 60 * total + (*it).toInt();
    
    return(total);
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

    return(s);
}
