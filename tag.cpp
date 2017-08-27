/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2009 Michael Pyne <mpyne@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tag.h"

#include <klocale.h>

#include <QtCore/QFile>

#include <taglib/tag.h>
#include <tfile.h>
#include <audioproperties.h>
#include <id3v2framefactory.h>

#include "cache.h"
#include "mediafiles.h"
#include "stringshare.h"
#include "juk_debug.h"

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
    if(fileName.isEmpty()) {
        qCCritical(JUK_LOG) << "Trying to add empty file";
        return;
    }

    TagLib::File *file = MediaFiles::fileFactoryByType(fileName);
    if(file && file->isValid()) {
        setup(file);
        delete file;
    }
    else {
        qCCritical(JUK_LOG) << "Couldn't resolve the mime type of \"" <<
            fileName << "\" -- this shouldn't happen." << endl;
    }
}

bool Tag::save()
{
    bool result;
    TagLib::ID3v2::FrameFactory::instance()->setDefaultTextEncoding(TagLib::String::UTF8);
    TagLib::File *file = MediaFiles::fileFactoryByType(m_fileName);

    if(file && !file->readOnly() && file->isValid() && file->tag()) {
        file->tag()->setTitle(TagLib::String(m_title.toUtf8().constData(), TagLib::String::UTF8));
        file->tag()->setArtist(TagLib::String(m_artist.toUtf8().constData(), TagLib::String::UTF8));
        file->tag()->setAlbum(TagLib::String(m_album.toUtf8().constData(), TagLib::String::UTF8));
        file->tag()->setGenre(TagLib::String(m_genre.toUtf8().constData(), TagLib::String::UTF8));
        file->tag()->setComment(TagLib::String(m_comment.toUtf8().constData(), TagLib::String::UTF8));
        file->tag()->setTrack(m_track);
        file->tag()->setYear(m_year);
        result = file->save();
    }
    else {
        qCCritical(JUK_LOG) << "Couldn't save file.";
        result = false;
    }

    delete file;
    return result;
}

QString Tag::playingString() const
{
    QString str;
    if(artist().isEmpty())
        str = title();
    else {
        str = i18nc("a playing track, %1 is artist, %2 is song title",
                    "%1 - <i>%2</i>", artist(), title());
    }

    return str;
}

CacheDataStream &Tag::read(CacheDataStream &s)
{
    switch(s.cacheVersion()) {
    case 1: {
        qint32 track;
        qint32 year;
        qint32 bitrate;
        qint32 seconds;

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

    minimizeMemoryUsage();
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
    if(!file || !file->tag()) {
        qCWarning(JUK_LOG) << "Can't setup invalid file" << m_fileName;
        return;
    }

    m_title   = TStringToQString(file->tag()->title()).trimmed();
    m_artist  = TStringToQString(file->tag()->artist()).trimmed();
    m_album   = TStringToQString(file->tag()->album()).trimmed();
    m_genre   = TStringToQString(file->tag()->genre()).trimmed();
    m_comment = TStringToQString(file->tag()->comment()).trimmed();

    m_track = file->tag()->track();
    m_year  = file->tag()->year();

    m_seconds = file->audioProperties()->length();
    m_bitrate = file->audioProperties()->bitrate();

    const int seconds = m_seconds % 60;
    const int minutes = (m_seconds - seconds) / 60;

    m_lengthString = QString::number(minutes) + (seconds >= 10 ? ":" : ":0") + QString::number(seconds);

    if(m_title.isEmpty()) {
        int i = m_fileName.lastIndexOf('/');
        int j = m_fileName.lastIndexOf('.');
        m_title = i > 0 ? m_fileName.mid(i + 1, j - i - 1) : m_fileName;
    }

    minimizeMemoryUsage();
    m_isValid = true;
}

void Tag::minimizeMemoryUsage()
{
    // Try to reduce memory usage: share tags that frequently repeat, squeeze others

    m_title.squeeze();
    m_lengthString.squeeze();

    m_comment = StringShare::tryShare(m_comment);
    m_artist  = StringShare::tryShare(m_artist);
    m_album   = StringShare::tryShare(m_album);
    m_genre   = StringShare::tryShare(m_genre);
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
      << qint32(t.track())
      << qint32(t.year())
      << t.comment()
      << qint32(t.bitrate())
      << t.lengthString()
      << qint32(t.seconds());

    return s;
}

CacheDataStream &operator>>(CacheDataStream &s, Tag &t)
{
    return t.read(s);
}

// vim: set et sw=4 tw=0 sta:
