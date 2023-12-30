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

#ifndef JUKTAG_H
#define JUKTAG_H

#include <QDateTime>

namespace TagLib { class File; }

class CacheDataStream;

/*!
 * This should really be called "metadata" and may at some point be titled as
 * such.  Right now it's mostly a Qt wrapper around TagLib.
 */

class Tag
{
    friend class FileHandle;
public:
    Tag(const QString &fileName);
    /**
     * Create an empty tag.  Used in FileHandle for cache restoration.
     */
    Tag(const QString &fileName, bool);

    bool save() const;

    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString genre() const { return m_genre; }
    int track() const { return m_track; }
    int year() const { return m_year; }
    QString comment() const { return m_comment; }

    QString fileName() const { return m_fileName; }

    void setTitle(const QString &value) { m_title = value; }
    void setArtist(const QString &value) { m_artist = value; }
    void setAlbum(const QString &value) { m_album = value; }
    void setGenre(const QString &value) { m_genre = value; }
    void setTrack(int value) { m_track = value; }
    void setYear(int value) { m_year = value; }
    void setComment(const QString &value) { m_comment = value; }

    void setFileName(const QString &value) { m_fileName = value; }

    int seconds() const { return m_seconds; }
    int bitrate() const { return m_bitrate; }

    bool isValid() const { return m_isValid; }

    /**
     * As a convenience, since producing a length string from a number of second
     * isn't a one liner, provide the length in string form.
     */
    QString lengthString() const { return m_lengthString; }

    /**
     * Convenience function to return a concise string describing the track,
     * usually in the form Artist - Title.
     */
    QString playingString() const;

    CacheDataStream &read(CacheDataStream &s);

private:
    void setup(TagLib::File *file);
    void minimizeMemoryUsage();

    QString m_fileName;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_genre;
    QString m_comment;
    int m_track;
    int m_year;
    int m_seconds;
    int m_bitrate;
    QDateTime m_modificationTime;
    QString m_lengthString;
    bool m_isValid;
};

QDataStream &operator<<(QDataStream &s, const Tag &t);
CacheDataStream &operator>>(CacheDataStream &s, Tag &t);

#endif

// vim: set et sw=4 tw=0 sta:
