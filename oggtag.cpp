/***************************************************************************
                          oggtag.cpp  -  description
                             -------------------
    begin                : Sat Oct 5 2002
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

#include <kdebug.h>

#include "oggtag.h"
#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

OggTag::OggTag(const QString &file) : Tag(file)
{
    metaInfo = KFileMetaInfo(file);
    commentGroup = KFileMetaInfoGroup(metaInfo.group("Comment"));
}

OggTag::~OggTag()
{

}

void OggTag::save()
{

}

QString OggTag::track() const
{
    return readCommentString("Album");
}

QString OggTag::artist() const
{
    return readCommentString("Artist");
}

QString OggTag::album() const
{
    return readCommentString("Album");
}

Genre OggTag::genre() const
{
    QString genreName = readCommentString("Genre");
    int index = GenreListList::ID3v1List()->findIndex(genreName);
    return Genre(genreName, index);
}

int OggTag::trackNumber() const
{
    return readCommentInt("Tracknumber");
}

QString OggTag::trackNumberString() const
{
    return readCommentString("Tracknumber");
}

int OggTag::year() const
{
    return readCommentInt("Year");
}

QString OggTag::yearString() const
{
    return readCommentString("Year");
}

QString OggTag::comment() const
{
    return(QString::null);
}

bool OggTag::hasTag() const
{
    if(metaInfo.isValid() && !metaInfo.isEmpty())
	return(true);
    else
	return(false);
}

void OggTag::setTrack(const QString &value)
{

}

void OggTag::setArtist(const QString &value)
{

}

void OggTag::setAlbum(const QString &value)
{

}

void OggTag::setGenre(const Genre &value)
{

}

void OggTag::setTrackNumber(int value)
{

}

void OggTag::setYear(int value)
{

}

void OggTag::setComment(const QString &value)
{

}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

QString OggTag::readCommentString(const QString &key) const
{
    if(metaInfo.isValid() && !metaInfo.isEmpty() &&
       commentGroup.isValid() && !commentGroup.isEmpty() &&
       commentGroup.contains(key))
	return(commentGroup.item(key).string());
    else
	return(QString::null);
}

int OggTag::readCommentInt(const QString &key) const
{
    if(metaInfo.isValid() && !metaInfo.isEmpty() &&
       commentGroup.isValid() && !commentGroup.isEmpty() &&
       commentGroup.contains(key)) {
	bool ok;
	int value = commentGroup.item(key).value().toInt(&ok);
	if(ok)
	    return(value);
	else
	    return(-1);
    }
    else
	return(-1);
}
