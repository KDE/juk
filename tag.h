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

/**
 * This class is an abstract base class for concrete Tag classes.  It provides
 * an API and a creation method to hide the differences between these.
 *
 * There are two parts here:  (1) those that are appropriately tag data, such as
 * the artist name or year recorded and (2) data that is not "tag" data but can 
 * benefit from the same abstraction mechanism used by the tag class.  This 
 * includes the track length and bitrate at the moment.
 */

class Tag
{
public:
    /**
     * All Tag objects should be instantiated through this method.  It determines
     * the appropriate concrete subclass and instantiates that.  It's servering
     * as a mini-factory; a full blown abstract factory is an overkill here.
     */
    static Tag *createTag(const QString &fileName, bool ignoreCache = false);
    virtual ~Tag();

    virtual void save();

    virtual QString track() const { return m_title; }
    virtual QString artist() const { return m_artist; }
    virtual QString album() const { return m_album; }
    virtual QString genre() const { return m_genre; }
    virtual int trackNumber() const { return m_track; }
    virtual QString trackNumberString() const { return QString::number(m_track); }
    virtual int year() const { return m_year; }
    virtual QString yearString() const { return QString::number(m_year); }
    virtual QString comment() const { return m_comment; }

    virtual void setTrack(const QString &value) { m_title = value; }
    virtual void setArtist(const QString &value) { m_artist = value; }
    virtual void setAlbum(const QString &value) { m_album = value; }
    virtual void setGenre(const QString &value) { m_genre = value; }
    virtual void setTrackNumber(int value) { m_track = value; }
    virtual void setYear(int value) { m_year = value; }
    virtual void setComment(const QString &value) { m_comment = value; }

    virtual QString bitrateString() const { return QString::number(m_bitrate); }
    virtual QString lengthString() const { return m_lengthString; }
    virtual int seconds() const { return m_seconds; }

    /**
     * Check to see if the item is up to date.  This defaults to true and should
     * be reimplemented inf Tag types that are not directly mapped to the file
     * system (specifically cached tags).
     */
    virtual bool current() const { return true; }

    // These functions are inlined because they are used on startup -- the most
    // performance critical section of JuK.

    inline QString absFilePath() const { return m_fileName; }
    inline QDateTime lastModified() const
	{ if(m_lastModified.isNull()) m_lastModified = m_info.lastModified(); return m_lastModified; }
    inline bool fileExists() const { return m_info.exists() && m_info.isFile(); }
    inline QFileInfo fileInfo() const { return m_info; }
    
protected:
    /**
     * The constructor is procetected since this is an abstract class and as
     * such it should not be instantiated directly.  createTag() should be
     * used to instantiate a concrete subclass of Tag that can be manipulated
     * though the public API.
     */
    Tag(const QString &file);

private:
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
};

QDataStream &operator<<(QDataStream &s, const Tag &t);

#endif
