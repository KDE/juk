/***************************************************************************
                      genre.cpp  -  description
                             -------------------
    begin                : Tue Feb 5 2002
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

#include "genre.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Genre::Genre() : QString(), m_ID3v1(255)
{

}

Genre::Genre(const QString &genreName, int ID3v1Number) : QString(genreName)
{
    m_ID3v1 = ID3v1Number;
}

// Ok, this just looks *really* ugly at first, but after thinking I must have
// been out of my mind when I originally wrote it; what it's doing is extracting
// some information from the object being overwritten, before it's overwritten.
//
// Basically it saves the "genre number" even across text assignments. 

Genre &Genre::operator=(const QString &genreName)
{
    Genre genre(genreName, getID3v1());
    *this = genre;
    return *this;
}

int Genre::getID3v1() const
{
    return m_ID3v1;
}

void Genre::setID3v1(int ID3v1Number)
{
    m_ID3v1 = ID3v1Number;
}

////////////////////////////////////////////////////////////////////////////////
// related functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const Genre &g)
{
    s << QString(g) << g.getID3v1();
    return s;
}

QDataStream &operator>>(QDataStream &s, Genre &g)
{
    QString name;
    int n;

    s >> name >> n;

    g = name;
    g.setID3v1(n);

    return s;
}
