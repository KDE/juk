/***************************************************************************
                          oggtag.cpp  -  description
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

#include "oggtag.h"

OggTag::OggTag(const QString &file) : Tag(file)
{
    
}

OggTag::~OggTag()
{

}

void OggTag::save()
{

}

QString OggTag::track() const
{
    return(QString::null);
}

QString OggTag::artist() const
{
    return(QString::null);
}

QString OggTag::album() const
{
    return(QString::null);
}

Genre OggTag::genre() const
{
    Genre g;
    return(g);
}

int OggTag::trackNumber() const
{
    return(0);
}

QString OggTag::trackNumberString() const
{
    return(QString::null);
}

int OggTag::year() const
{
    return(0);
}

QString OggTag::yearString() const
{
    return(QString::null);
}

QString OggTag::comment() const
{
    return(QString::null);
}

bool OggTag::hasTag() const
{
    return(false);
}

void OggTag::setTrack(const QString &value)
{

}

void OggTag::setArtist(const QString &value)
{

}

void OggTag::setAlbum(const QString &value)
{

}

void OggTag::setGenre(const Genre &value)
{

}

void OggTag::setTrackNumber(int value)
{

}

void OggTag::setYear(int value)
{

}

void OggTag::setComment(const QString &value)
{

}
