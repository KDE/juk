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

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

Tag::Tag(const QString &file) : m_info(file), m_fileName(file)
{
    Cache::instance()->insert(file, this);
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

Tag::Tag(const QString &fileName, TagLib::File *file) :
    m_info(fileName), m_fileName(fileName)
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
      << t.trackNumberString()
      << t.year()
      << t.yearString()
      << t.comment()
      << t.bitrateString()
      << t.lengthString()
      << t.seconds()
      << QString::null             // TODO: remove
      << t.lastModified();

    return s;
}
