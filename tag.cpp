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
#include <qfile.h>

#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/xiphcomment.h>

#include "cache.h"
#include "tag.h"
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
        TagLib::MPEG::File file(QFile::encodeName(fileName).data());
        if(!file.isValid())
            return 0;
        return new Tag(fileName, &file);
    }

    if(MediaFiles::isOgg(fileName)) {
        TagLib::Vorbis::File file(QFile::encodeName(fileName).data());
        if(!file.isValid())
            return 0;
        return new Tag(fileName, &file);
    }

    kdError(65432) << "Couldn't resolve the mime type of \"" <<
        fileName << "\" -- this shouldn't happen." << endl;

    return 0;
}

Tag::~Tag()
{
    Cache::instance()->remove(m_fileName);
}

void Tag::save()
{
    if(!m_info.isWritable())
        return;

    TagLib::File *file = 0;

    if(MediaFiles::isMP3(m_fileName))
        file = new TagLib::MPEG::File(QFile::encodeName(m_fileName).data());
#if defined TAGLIB_MINOR_VERSION && TAGLIB_MINOR_VERSION >= 95
    else if(MediaFiles::isOgg(m_fileName))
        file = new TagLib::Vorbis::File(QFile::encodeName(m_fileName).data());
#else
#ifdef _GNUC
#warning "Your TagLib is too old for saving Vorbis files.  It is being disabled for now."
#endif
#endif

    if(file && file->isValid() && file->tag()) {
        file->tag()->setTitle(QStringToTString(m_title));
        file->tag()->setArtist(QStringToTString(m_artist));
        file->tag()->setAlbum(QStringToTString(m_album));
        file->tag()->setGenre(QStringToTString(m_genre));
        file->tag()->setComment(QStringToTString(m_comment));
        file->tag()->setTrack(m_track);
        file->tag()->setYear(m_year);

        file->save();
    }

    delete file;
}

bool Tag::current() const
{
    return(m_modificationTime.isValid() &&
           lastModified().isValid() &&
           m_modificationTime >= Tag::lastModified());
}

QDateTime Tag::lastModified() const
{
    if(m_lastModified.isNull())
        m_lastModified = m_info.lastModified();
    return m_lastModified;
}


CacheDataStream &Tag::read(CacheDataStream &s)
{
    switch(s.cacheVersion()) {
    case 1: {
        Q_INT32 track;
        Q_INT32 year;
        Q_INT32 bitrate;
        Q_INT32 seconds;

        s >> m_title
          >> m_artist
          >> m_album
          >> m_genre
          >> track
          >> year
          >> m_comment
          >> bitrate
          >> m_lengthString
          >> seconds
          >> m_modificationTime;

        m_track = track;
        m_year = year;
        m_bitrate = bitrate;
        m_seconds = seconds;
        break;
    }
    default: {
        static QString dummyString;
        static int dummyInt;
        QString bitrateString;

        s >> dummyInt
          >> m_title
          >> m_artist
          >> m_album
          >> m_genre
          >> dummyInt
          >> m_track
          >> dummyString
          >> m_year
          >> dummyString
          >> m_comment
          >> bitrateString
          >> m_lengthString
          >> m_seconds
          >> dummyString
          >> m_modificationTime;

        bool ok;
        m_bitrate = bitrateString.toInt(&ok);
        if(!ok)
            m_bitrate = 0;
        break;
    }
    }

    // Try to reduce memory usage: share tags that frequently repeat, squeeze others

    m_title.squeeze();
    m_lengthString.squeeze();

    m_comment = StringShare::tryShare(m_comment);
    m_artist  = StringShare::tryShare(m_artist);
    m_album   = StringShare::tryShare(m_album);
    m_genre   = StringShare::tryShare(m_genre);

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
    m_title   = TStringToQString(file->tag()->title()).stripWhiteSpace();
    m_artist  = TStringToQString(file->tag()->artist()).stripWhiteSpace();
    m_album   = TStringToQString(file->tag()->album()).stripWhiteSpace();
    m_genre   = TStringToQString(file->tag()->genre()).stripWhiteSpace();
    m_comment = TStringToQString(file->tag()->comment()).stripWhiteSpace();

    m_track = file->tag()->track();
    m_year  = file->tag()->year();

    m_seconds = file->audioProperties()->length();
    m_bitrate = file->audioProperties()->length();

    const int seconds = m_seconds % 60;
    const int minutes = (m_seconds - seconds) / 60;

    m_lengthString = QString::number(minutes) + (seconds >= 10 ? ":" : ":0") + QString::number(seconds);

    if(m_title.isEmpty())
        m_title = m_info.baseName(true);

    Cache::instance()->insert(fileName, this);
}

////////////////////////////////////////////////////////////////////////////////
// related functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const Tag &t)
{
    s << t.title()
      << t.artist()
      << t.album()
      << t.genre()
      << Q_INT32(t.track())
      << Q_INT32(t.year())
      << t.comment()
      << Q_INT32(t.bitrate())
      << t.lengthString()
      << Q_INT32(t.seconds())
      << t.lastModified();

    return s;
}

CacheDataStream &operator>>(CacheDataStream &s, Tag &t)
{
    return t.read(s);
}
