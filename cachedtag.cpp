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

#if 0 /// TODO: This should be included the next time that the cache format changes
#include <kdatastream.h>
#endif

#include "cachedtag.h"
#include "cache.h"
#include "stringshare.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

CachedTag::CachedTag(const QString &file) : Tag(file), 
					    m_externalTag(0), m_tagTrackNumber(0),
					    m_tagYear(0), m_tagSeconds(0)
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

QString CachedTag::genre() const
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

void CachedTag::setGenre(const QString &value)
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
    static QString dummyString;
    static int dummyInt;

    // TODO: Use Q_UINT32 in place of all integers.

    s >> dummyInt                // TODO: remove
      >> m_tagTrack
      >> m_tagArtist
      >> m_tagAlbum
      >> m_tagGenre
      >> dummyInt                // TODO: remove
      >> m_tagTrackNumber
      >> m_tagTrackNumberString
      >> m_tagYear
      >> m_tagYearString
      >> m_tagComment
      >> m_tagBitrateString
      >> m_tagLengthString
      >> m_tagSeconds
      >> dummyString             // TODO: remove
      >> m_modificationTime;

    // Try to reduce memory usage: share tags that frequently repeat, squeeze others            
    m_tagTrack.squeeze();
    m_tagComment = StringShare::tryShare(m_tagComment);
    m_tagArtist  = StringShare::tryShare(m_tagArtist);
    m_tagAlbum   = StringShare::tryShare(m_tagAlbum);
    m_tagTrackNumberString = StringShare::tryShare(m_tagTrackNumberString);
    m_tagYearString        = StringShare::tryShare(m_tagYearString);
    m_tagBitrateString     = StringShare::tryShare(m_tagBitrateString);
    m_tagLengthString.squeeze();
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
