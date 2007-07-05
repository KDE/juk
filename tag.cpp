/***************************************************************************
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
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
#include <taglib/flacfile.h>
#include <taglib/xiphcomment.h>
#include <taglib/id3v2framefactory.h>

#if (TAGLIB_MAJOR_VERSION > 1) || \
      ((TAGLIB_MAJOR_VERSION == 1) && (TAGLIB_MINOR_VERSION >= 2))
#include <taglib/oggflacfile.h>
#define TAGLIB_1_2
#endif
#if (TAGLIB_MAJOR_VERSION > 1) || \
    ((TAGLIB_MAJOR_VERSION == 1) && (TAGLIB_MINOR_VERSION >= 3))
#include <taglib/mpcfile.h>
#define TAGLIB_1_3
#endif

#include "cache.h"
#include "tag.h"
#include "mediafiles.h"
#include "stringshare.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////


Tag::Tag(const QString &fileName) :
    m_fileName(fileName),
    m_track(0),
    m_year(0),
    m_seconds(0),
    m_bitrate(0),
    m_isValid(false)
{
    // using qDebug here since we want this to show up in non-debug builds as well

    qDebug("Reading tag for %s", fileName.local8Bit().data());

    if(MediaFiles::isMP3(fileName)) {
        TagLib::MPEG::File file(QFile::encodeName(fileName).data());
        if(file.isValid())
            setup(&file);
    }

    else if(MediaFiles::isFLAC(fileName)) {
        TagLib::FLAC::File file(QFile::encodeName(fileName).data());
        if(file.isValid())
            setup(&file);
    }
#ifdef TAGLIB_1_3
    else if(MediaFiles::isMPC(fileName)) {
        kdDebug(65432) << "Trying to resolve Musepack file" << endl;
        TagLib::MPC::File file(QFile::encodeName(fileName).data());
        if(file.isValid())
            setup(&file);
    }
#endif
#ifdef TAGLIB_1_2
    else if(MediaFiles::isOggFLAC(fileName)) {
        TagLib::Ogg::FLAC::File file(QFile::encodeName(fileName).data());
        if(file.isValid())
            setup(&file);
    }
#endif
    else if(MediaFiles::isVorbis(fileName)) {
        TagLib::Vorbis::File file(QFile::encodeName(fileName).data());
        if(file.isValid())
            setup(&file);
    }

    else {
        kdError(65432) << "Couldn't resolve the mime type of \"" <<
            fileName << "\" -- this shouldn't happen." << endl;
    }
}

bool Tag::save()
{
    bool result;
    TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF8);

    TagLib::File *file = 0;

    if(MediaFiles::isMP3(m_fileName))
        file = new TagLib::MPEG::File(QFile::encodeName(m_fileName).data());
    else if(MediaFiles::isFLAC(m_fileName))
        file = new TagLib::FLAC::File(QFile::encodeName(m_fileName).data());
#ifdef TAGLIB_1_3
    else if(MediaFiles::isMPC(m_fileName))
        file = new TagLib::MPC::File(QFile::encodeName(m_fileName).data());
#endif
#ifdef TAGLIB_1_2
    else if(MediaFiles::isOggFLAC(m_fileName))
        file = new TagLib::Ogg::FLAC::File(QFile::encodeName(m_fileName).data());
#endif
    else if(MediaFiles::isVorbis(m_fileName))
        file = new TagLib::Vorbis::File(QFile::encodeName(m_fileName).data());

    if(file && file->isValid() && file->tag() && !file->readOnly()) {
        file->tag()->setTitle(QStringToTString(m_title));
        file->tag()->setArtist(QStringToTString(m_artist));
        file->tag()->setAlbum(QStringToTString(m_album));
        file->tag()->setGenre(QStringToTString(m_genre));
        file->tag()->setComment(QStringToTString(m_comment));
        file->tag()->setTrack(m_track);
        file->tag()->setYear(m_year);
#ifdef TAGLIB_1_2
        result = file->save();
#else
        file->save();
        result = true;
#endif
    }
    else {
        kdError(65432) << "Couldn't save file." << endl;
        result = false;
    }

    delete file;
    return result;
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
          >> seconds;

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
          >> dummyString;

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

Tag::Tag(const QString &fileName, bool) :
    m_fileName(fileName),
    m_track(0),
    m_year(0),
    m_seconds(0),
    m_bitrate(0),
    m_isValid(true)
{

}

void Tag::setup(TagLib::File *file)
{
    m_title   = TStringToQString(file->tag()->title()).stripWhiteSpace();
    m_artist  = TStringToQString(file->tag()->artist()).stripWhiteSpace();
    m_album   = TStringToQString(file->tag()->album()).stripWhiteSpace();
    m_genre   = TStringToQString(file->tag()->genre()).stripWhiteSpace();
    m_comment = TStringToQString(file->tag()->comment()).stripWhiteSpace();

    m_track = file->tag()->track();
    m_year  = file->tag()->year();

    m_seconds = file->audioProperties()->length();
    m_bitrate = file->audioProperties()->bitrate();

    const int seconds = m_seconds % 60;
    const int minutes = (m_seconds - seconds) / 60;

    m_lengthString = QString::number(minutes) + (seconds >= 10 ? ":" : ":0") + QString::number(seconds);

    if(m_title.isEmpty()) {
        int i = m_fileName.findRev('/');
        int j = m_fileName.findRev('.');
        m_title = i > 0 ? m_fileName.mid(i + 1, j - i - 1) : m_fileName;
    }

    m_isValid = true;
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
      << Q_INT32(t.seconds());

    return s;
}

CacheDataStream &operator>>(CacheDataStream &s, Tag &t)
{
    return t.read(s);
}
