/***************************************************************************
                          tag.cpp  -  description
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

#include <qfileinfo.h>

#include "tag.h"
#include "id3tag.h"
#include "oggtag.h"
#include "cachedtag.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Tag *Tag::createTag(const QString &file)
{
    QFileInfo f(file);
    QString extension = f.extension(false).lower();
    
    // insert a check for a cache hit here

    if(extension == "mp3")
	return new ID3Tag(file);
    if(extension == "ogg")
	return new OggTag(file);
    else
	return(0);
}

Tag::~Tag()
{

}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

Tag::Tag(const QString &file)
{

}
