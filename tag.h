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

#include "genre.h"

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

    virtual void save() = 0;
    virtual bool hasTag() const = 0;

    virtual QString track() const = 0;
    virtual QString artist() const = 0;
    virtual QString album() const = 0;
    virtual Genre genre() const = 0;
    virtual int trackNumber() const = 0;
    virtual QString trackNumberString() const = 0;
    virtual int year() const = 0;
    virtual QString yearString() const = 0;
    virtual QString comment() const = 0;

    virtual void setTrack(const QString &value) = 0;
    virtual void setArtist(const QString &value) = 0;
    virtual void setAlbum(const QString &value) = 0;
    virtual void setGenre(const Genre &value) = 0;
    virtual void setTrackNumber(int value) = 0;
    virtual void setYear(int value) = 0;
    virtual void setComment(const QString &value) = 0;

    virtual QString bitrateString() const = 0;
    virtual QString lengthString() const = 0;
    virtual int seconds() const = 0;

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

    static QString readBitrate(const KFileMetaInfo &metaInfo);
    static QString readLength(const KFileMetaInfo &metaInfo);
    static int readSeconds(const KFileMetaInfo &metaInfo);
    
private:
    QFileInfo m_info;
    QString m_fileName;
    mutable QDateTime m_lastModified;
};

QDataStream &operator<<(QDataStream &s, const Tag &t);

#endif
