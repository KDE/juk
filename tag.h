/***************************************************************************
                          id3tag.h  -  description
                             -------------------
    begin                : Sun Feb 17 2002
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

#ifndef TAG_H
#define TAG_H

#include <qstring.h>

#include <id3/tag.h>

#include "genre.h"

class Tag
{
public:
    /**
     * All Tag objects should be instantiated through this method.  It determines
     * the appropriate concrete subclass and instantiates that.  It's servering
     * as a mini-factory; a full blown abstract factory is an overkill here.
     */
    static Tag *createTag(const QString &file);
    virtual ~Tag();

    virtual void save() = 0;

    virtual QString track() const = 0;
    virtual QString artist() const = 0;
    virtual QString album() const = 0;
    virtual Genre genre() const = 0;
    virtual int trackNumber() const = 0;
    virtual QString trackNumberString() const = 0;
    virtual int year() const = 0;
    virtual QString yearString() const = 0;
    virtual QString comment() const = 0;
    virtual bool hasTag() const = 0;

    virtual void setTrack(const QString &value) = 0;
    virtual void setArtist(const QString &value) = 0;
    virtual void setAlbum(const QString &value) = 0;
    virtual void setGenre(const Genre &value) = 0;
    virtual void setTrackNumber(int value) = 0;
    virtual void setYear(int value) = 0;
    virtual void setComment(const QString &value) = 0;
    
protected:
    /**
     * The constructor is procetected since this is an abstract class and as
     * such it should not be instantiated directly.  createTag() should be
     * used to instantiate a concrete subclass of Tag that can be manipulated
     * though the public API.
     *
     * Here we're also accepting a file as a parameter, not because it is used
     * now, but reserving it for later use.
     */
    Tag(const QString &file);
};

#endif
