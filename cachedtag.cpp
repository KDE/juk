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

CachedTag::CachedTag(const QString &file) : Tag(file), 
					    m_externalTag(0), m_tagTrackNumber(0), m_tagYear(0), m_tagSeconds(0), m_tagExists(false)
{

}

CachedTag::~CachedTag()
{
    delete m_externalTag;
}

void CachedTag::save()
{
    if(m_externalTag)
	m_externalTag->save();
}

QString CachedTag::track() const
{
    if(m_externalTag)
	return m_externalTag->track();
    else
	return m_tagTrack;
}

QString CachedTag::artist() const
{
    if(m_externalTag)
	return m_externalTag->artist();
    else
	return m_tagArtist;
}

QString CachedTag::album() const
{
    if(m_externalTag)
	return m_externalTag->album();
    else
	return m_tagAlbum;
}

Genre CachedTag::genre() const
{
    if(m_externalTag)
	return m_externalTag->genre();
    else
	return m_tagGenre;
}

int CachedTag::trackNumber() const
{
    if(m_externalTag)
	return m_externalTag->trackNumber();
    else
	return m_tagTrackNumber;
}

QString CachedTag::trackNumberString() const
{
    if(m_externalTag)
	return m_externalTag->trackNumberString();
    else
	return m_tagTrackNumberString;
}

int CachedTag::year() const
{
    if(m_externalTag)
	return m_externalTag->year();
    else
	return m_tagYear;
}

QString CachedTag::yearString() const
{
    if(m_externalTag)
	return m_externalTag->yearString();
    else
	return m_tagYearString;
}

QString CachedTag::comment() const
{
    if(m_externalTag)
	return m_externalTag->comment();
    else
	return m_tagComment;
}

bool CachedTag::hasTag() const
{
    if(m_externalTag)
	return m_externalTag->hasTag();
    else
	return m_tagExists;
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
    return m_tagBitrateString;
}

QString CachedTag::lengthString() const
{
    return m_tagLengthString;
}

int CachedTag::seconds() const
{
    return m_tagSeconds;
}

bool CachedTag::current() const
{
    return(m_modificationTime.isValid() && 
	   lastModified().isValid() &&
	   m_modificationTime >= Tag::lastModified());
}

QDataStream &CachedTag::read(QDataStream &s)
{
    s >> int(m_tagExists)

      >> m_tagTrack
      >> m_tagArtist
      >> m_tagAlbum
      >> m_tagGenre
      >> m_tagTrackNumber
      >> m_tagTrackNumberString
      >> m_tagYear
      >> m_tagYearString
      >> m_tagComment

      >> m_tagBitrateString
      >> m_tagLengthString
      >> m_tagSeconds

      >> m_fileName
      >> m_modificationTime;

    return s;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

Tag *CachedTag::proxiedTag()
{
    if(!m_externalTag) {
	Cache::instance()->remove(absFilePath());
	m_externalTag = Tag::createTag(absFilePath(), true);
    }

    return m_externalTag;
}

////////////////////////////////////////////////////////////////////////////////
// related functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator>>(QDataStream &s, CachedTag &t)
{
    return t.read(s);
}
