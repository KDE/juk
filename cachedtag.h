/***************************************************************************
                          cachedtag.h  -  description
                             -------------------
    begin                : Sat Oct 5 2002
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

#ifndef CACHEDTAG_H
#define CACHEDTAG_H

#include <qdatastream.h>

#include "tag.h"

class CachedTag : public Tag
{
public: 
    CachedTag(const QString &file);
    virtual ~CachedTag();

    virtual void save();
    virtual bool hasTag() const;

    virtual QString track() const;
    virtual QString artist() const;
    virtual QString album() const;
    virtual Genre genre() const;
    virtual int trackNumber() const;
    virtual QString trackNumberString() const;
    virtual int year() const;
    virtual QString yearString() const;
    virtual QString comment() const;

    virtual void setTrack(const QString &value);
    virtual void setArtist(const QString &value);
    virtual void setAlbum(const QString &value);
    virtual void setGenre(const Genre &value);
    virtual void setTrackNumber(int value);
    virtual void setYear(int value);
    virtual void setComment(const QString &value);

    virtual QString bitrateString() const;
    virtual QString lengthString() const;
    virtual int seconds() const;

    // CachedTag specific methods

    /**
     * Checks to see if the cache for this item is up to date.
     */
    virtual bool current() const;
    QDataStream &read(QDataStream &s);

private:
    Tag *proxiedTag();
    Tag *m_externalTag;

    QString m_tagTrack;
    QString m_tagArtist;
    QString m_tagAlbum;
    Genre m_tagGenre;
    int m_tagTrackNumber;
    QString m_tagTrackNumberString;
    int m_tagYear;
    QString m_tagYearString;
    QString m_tagComment;

    QString m_tagBitrateString;
    QString m_tagLengthString;
    int m_tagSeconds;

    bool m_tagExists;

    QDateTime m_modificationTime;
};

QDataStream &operator>>(QDataStream &s, CachedTag &t);

#endif
