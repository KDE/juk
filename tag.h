/***************************************************************************
                          tag.h  -  description
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

#ifndef TAG_H
#define TAG_H

#include <qfileinfo.h>

namespace TagLib { class File; }

class CacheDataStream;

/*!
 * This should really be called "metadata" and may at some point be titled as
 * such.  Right now it's mostly a Qt wrapper around TagLib.
 */

class Tag
{
    friend class Cache;
    friend class FileHandle;
public:
    /**
     * All Tag objects should be instantiated through this method.  It determines
     * the appropriate concrete subclass and instantiates that.  It's servering
     * as a mini-factory; a full blown abstract factory is an overkill here.
     */
    static Tag *createTag(const QString &fileName, bool ignoreCache = false);

    ~Tag();

    void save();

    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString genre() const { return m_genre; }
    int track() const { return m_track; }
    int year() const { return m_year; }
    QString comment() const { return m_comment; }

    void setTitle(const QString &value) { m_title = value; }
    void setArtist(const QString &value) { m_artist = value; }
    void setAlbum(const QString &value) { m_album = value; }
    void setGenre(const QString &value) { m_genre = value; }
    void setTrack(int value) { m_track = value; }
    void setYear(int value) { m_year = value; }
    void setComment(const QString &value) { m_comment = value; }

    int seconds() const { return m_seconds; }
    int bitrate() const { return m_bitrate; }

    /**
     * As a convenience, since producing a length string from a number of second
     * isn't a one liner, provide the lenght in string form.
     */
    QString lengthString() const { return m_lengthString; }
    CacheDataStream &read(CacheDataStream &s);

    // TODO -- REMOVE THESE METHODS ONCE THE CACHE IS FILEHANDLE BASED
    const QDateTime &lastModified() const;
    const QFileInfo &fileInfo() const { return m_info; }
    const QString &fileName() const { return m_fileName; }

private:
    /*!
     * Creates an empty tag for use in Cache restoration.
     */
    Tag(const QString &file);
    Tag(const QString &fileName, TagLib::File *file);

    // TODO -- remove m_info and prefer the one in the FileHandle

    QFileInfo m_info;
    QString m_fileName;
    mutable QDateTime m_lastModified;
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
};

QDataStream &operator<<(QDataStream &s, const Tag &t);
CacheDataStream &operator>>(CacheDataStream &s, Tag &t);

#endif
