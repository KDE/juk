/***************************************************************************
                          id3tag.h  -  description
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

#include <kfilemetainfo.h>

#include <qstring.h>
#include <qdatetime.h>
#include <qfileinfo.h>

namespace TagLib { class File; }

/*!
 * This should really be called "metadata" and may at some point be titled as
 * such.  Right now it's mostly a Qt wrapper around TagLib.
 */

class Tag
{
    friend class Cache;
public:
    /**
     * All Tag objects should be instantiated through this method.  It determines
     * the appropriate concrete subclass and instantiates that.  It's servering
     * as a mini-factory; a full blown abstract factory is an overkill here.
     */
    static Tag *createTag(const QString &fileName, bool ignoreCache = false);

    ~Tag();

    void save();

    QString track() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString genre() const { return m_genre; }
    int trackNumber() const { return m_track; }
    QString trackNumberString() const { return QString::number(m_track); }
    int year() const { return m_year; }
    QString yearString() const { return QString::number(m_year); }
    QString comment() const { return m_comment; }

    void setTrack(const QString &value) { m_title = value; }
    void setArtist(const QString &value) { m_artist = value; }
    void setAlbum(const QString &value) { m_album = value; }
    void setGenre(const QString &value) { m_genre = value; }
    void setTrackNumber(int value) { m_track = value; }
    void setYear(int value) { m_year = value; }
    void setComment(const QString &value) { m_comment = value; }

    QString bitrateString() const { return m_bitrateString; }
    QString lengthString() const { return m_lengthString; }
    int seconds() const { return m_seconds; }

    /**
     * Check to see if the item is up to date.
     */
    bool current() const;

    // These functions are inlined because they are used on startup -- the most
    // performance critical section of JuK.

    inline QString absFilePath() const { return m_fileName; }
    inline QDateTime lastModified() const
    {
        if(m_lastModified.isNull())
            m_lastModified = m_info.lastModified();
        return m_lastModified;
    }
    inline bool fileExists() const { return m_info.exists() && m_info.isFile(); }
    inline QFileInfo fileInfo() const { return m_info; }

    QDataStream &read(QDataStream &s);

private:
    /*!
     * Creates an empty tag for use in Cache restoration.
     */
    Tag(const QString &file);
    Tag(const QString &fileName, TagLib::File *file);

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
    QString m_lengthString;
    QString m_bitrateString;
    QDateTime m_modificationTime;
};

QDataStream &operator<<(QDataStream &s, const Tag &t);
QDataStream &operator>>(QDataStream &s, Tag &t);

#endif
