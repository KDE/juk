/***************************************************************************
                          oggtag.h  -  description
                             -------------------
    begin                : Sat Oct 5 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OGGTAG_H
#define OGGTAG_H

#include "tag.h"

class OggTag : public Tag
{
public: 
    OggTag(const QString &file);
    virtual ~OggTag();

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

private:
    /**
     * Simplifies reading a string from a KFMI group.  Returns QString::null if
     * something went wrong or the key was not found.
     */
    QString readCommentString(const QString &key) const;
    /**
     * Simplifies reading an int from a KFMI group.  Returns -1 if something went
     * wrong or the key was not found.
     */
    int readCommentInt(const QString &key) const;
    /**
     * Writes a string to the specified key.
     */
    void writeCommentItem(const QString &key, const QString &value);
    /**
     * Writes a string to the specified key.
     */
    void writeCommentItem(const QString &key, int value);
    
    KFileMetaInfo metaInfo;
    KFileMetaInfoGroup commentGroup;
};

#endif
