/***************************************************************************
                          cachedtag.cpp  -  description
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

#include "cachedtag.h"

CachedTag::CachedTag(const QString &file) : Tag(file)
{

}

CachedTag::~CachedTag()
{

}

void CachedTag::save()
{

}

QString CachedTag::track() const
{
    return(QString::null);
}

QString CachedTag::artist() const
{
    return(QString::null);
}

QString CachedTag::album() const
{
    return(QString::null);
}

Genre CachedTag::genre() const
{
    Genre g;
    return(g);
}

int CachedTag::trackNumber() const
{
    return(0);
}

QString CachedTag::trackNumberString() const
{
    return(QString::null);
}

int CachedTag::year() const
{
    return(0);
}

QString CachedTag::yearString() const
{
    return(QString::null);
}

QString CachedTag::comment() const
{
    return(QString::null);
}

bool CachedTag::hasTag() const
{
    return(false);
}

void CachedTag::setTrack(const QString &value)
{

}

void CachedTag::setArtist(const QString &value)
{

}

void CachedTag::setAlbum(const QString &value)
{

}

void CachedTag::setGenre(const Genre &value)
{

}

void CachedTag::setTrackNumber(int value)
{

}

void CachedTag::setYear(int value)
{

}

void CachedTag::setComment(const QString &value)
{

}
