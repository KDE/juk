/***************************************************************************
                          cachedtag.cpp  -  description
                             -------------------
    begin                : Sat Oct 5 2002
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

#include "cachedtag.h"
#include "cache.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

CachedTag::CachedTag(const QString &file) : Tag(file), externalTag(0), tagYear(0), tagTrackNumber(0), tagSeconds(0), tagExists(false)
{

}

CachedTag::~CachedTag()
{
    delete(externalTag);
}

void CachedTag::save()
{
    if(externalTag)
	externalTag->save();
}

QString CachedTag::track() const
{
    if(externalTag)
	return(externalTag->track());
    else
	return(tagTrack);
}

QString CachedTag::artist() const
{
    if(externalTag)
	return(externalTag->artist());
    else
	return(tagArtist);
}

QString CachedTag::album() const
{
    if(externalTag)
	return(externalTag->album());
    else
	return(tagAlbum);
}

Genre CachedTag::genre() const
{
    if(externalTag)
	return(externalTag->genre());
    else
	return(tagGenre);
}

int CachedTag::trackNumber() const
{
    if(externalTag)
	return(externalTag->trackNumber());
    else
	return(tagTrackNumber);
}

QString CachedTag::trackNumberString() const
{
    if(externalTag)
	return(externalTag->trackNumberString());
    else
	return(tagTrackNumberString);
}

int CachedTag::year() const
{
    if(externalTag)
	return(externalTag->year());
    else
	return(tagYear);
}

QString CachedTag::yearString() const
{
    if(externalTag)
	return(externalTag->yearString());
    else
	return(tagYearString);
}

QString CachedTag::comment() const
{
    if(externalTag)
	return(externalTag->comment());
    else
	return(tagComment);
}

bool CachedTag::hasTag() const
{
    if(externalTag)
	return(externalTag->hasTag());
    else
	return(tagExists);
}

void CachedTag::setTrack(const QString &value)
{
    proxiedTag()->setTrack(value);
}

void CachedTag::setArtist(const QString &value)
{
    proxiedTag()->setArtist(value);
}

void CachedTag::setAlbum(const QString &value)
{
    proxiedTag()->setAlbum(value);
}

void CachedTag::setGenre(const Genre &value)
{
    proxiedTag()->setGenre(value);
}

void CachedTag::setTrackNumber(int value)
{
    proxiedTag()->setTrackNumber(value);
}

void CachedTag::setYear(int value)
{
    proxiedTag()->setYear(value);
}

void CachedTag::setComment(const QString &value)
{
    proxiedTag()->setComment(value);
}

QString CachedTag::bitrateString() const
{
    return(tagBitrateString);
}

QString CachedTag::lengthString() const
{
    return(tagLengthString);
}

int CachedTag::seconds() const
{
    return(tagSeconds);
}

bool CachedTag::current() const
{
    return(fileExists() &&
	   modificationTime.isValid() && 
	   lastModified().isValid() &&
	   modificationTime >= Tag::lastModified());
}

QDataStream &CachedTag::read(QDataStream &s)
{
    s >> int(tagExists)

      >> tagTrack
      >> tagArtist
      >> tagAlbum
      >> tagGenre
      >> tagTrackNumber
      >> tagTrackNumberString
      >> tagYear
      >> tagYearString
      >> tagComment

      >> tagBitrateString
      >> tagLengthString
      >> tagSeconds

      >> fileName
      >> modificationTime;

    return(s);
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

Tag *CachedTag::proxiedTag()
{
    if(!externalTag) {
	Cache::instance()->remove(absFilePath());
	externalTag = Tag::createTag(absFilePath(), true);
    }

    return(externalTag);
}

////////////////////////////////////////////////////////////////////////////////
// related functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator>>(QDataStream &s, CachedTag &t)
{
    return t.read(s);
}
