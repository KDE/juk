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

#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/xiphcomment.h>

#include "tag.h"
#include "cache.h"
#include "mediafiles.h"
#include "stringshare.h"

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

    if(MediaFiles::isMP3(fileName)) {
        TagLib::MPEG::File file(QStringToTString(fileName));
        return new Tag(fileName, &file);
    }

    if(MediaFiles::isOgg(fileName)) {
        TagLib::Vorbis::File file(QStringToTString(fileName));
        return new Tag(fileName, &file);
    }

    kdError(65432) << "Couldn't resolve the mime type of \"" <<
        fileName << "\" -- this shouldn't happen." << endl;

    return 0;
}

Tag::~Tag()
{
    Cache::instance()->remove(absFilePath());
}

void Tag::save()
{

}

bool Tag::current() const
{
    return(m_modificationTime.isValid() &&
           lastModified().isValid() &&
           m_modificationTime >= Tag::lastModified());
}

QDataStream &Tag::read(QDataStream &s)
{
    static QString dummyString;
    static int dummyInt;

    // TODO: Use Q_UINT32 in place of all integers.

    s >> dummyInt                // TODO: remove
      >> m_title
      >> m_artist
      >> m_album
      >> m_genre
      >> dummyInt                // TODO: remove
      >> m_track
      >> dummyString             // TODO: remove
      >> m_year
      >> dummyString             // TODO: remove
      >> m_comment
      >> m_bitrateString         // TODO: remove and replace with int
      >> m_lengthString
      >> m_seconds
      >> dummyString             // TODO: remove
      >> m_modificationTime;

    // Try to reduce memory usage: share tags that frequently repeat, squeeze others
    m_title.squeeze();
    m_comment       = StringShare::tryShare(m_comment);
    m_artist        = StringShare::tryShare(m_artist);
    m_album         = StringShare::tryShare(m_album);
    m_genre         = StringShare::tryShare(m_genre);
    m_bitrateString = StringShare::tryShare(m_bitrateString);
    m_lengthString.squeeze();
    return s;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

Tag::Tag(const QString &file) :
    m_info(file),
    m_fileName(file),
    m_track(0),
    m_year(0),
    m_seconds(0),
    m_bitrate(0)
{
    Cache::instance()->insert(file, this);
}

Tag::Tag(const QString &fileName, TagLib::File *file) :
    m_info(fileName),
    m_fileName(fileName)
{
    m_title   = TStringToQString(file->tag()->title());
    m_artist  = TStringToQString(file->tag()->artist());
    m_album   = TStringToQString(file->tag()->album());
    m_genre   = TStringToQString(file->tag()->genre());
    m_comment = TStringToQString(file->tag()->comment());

    m_track = file->tag()->track();
    m_year  = file->tag()->year();

    m_seconds = file->audioProperties()->length();
    m_bitrate = file->audioProperties()->length();
    m_bitrateString = QString::number(m_bitrate);

    const int seconds = m_seconds % 60;
    const int minutes = (m_seconds - seconds) / 60;

    m_lengthString = QString::number(minutes) + (seconds >= 10 ? ":" : ":0") + QString::number(seconds);

    Cache::instance()->insert(fileName, this);
}

////////////////////////////////////////////////////////////////////////////////
// related functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const Tag &t)
{
    // TODO: Use Q_UINT32 in place of all integers.

    s << 0                         // TODO: remove
      << t.track()
      << t.artist()
      << t.album()
      << t.genre()
      << 0                         // TODO: remove
      << t.trackNumber()
      << QString::null             // TODO: remove
      << t.year()
      << QString::null             // TODO: remove
      << t.comment()
      << t.bitrateString()         // TODO: remove and replace with int
      << t.lengthString()
      << t.seconds()
      << QString::null             // TODO: remove
      << t.lastModified();

    return s;
}

QDataStream &operator>>(QDataStream &s, Tag &t)
{
    return t.read(s);
}
