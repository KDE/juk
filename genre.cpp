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

Genre::Genre() : 
    m_ID3v1(255)
{

}

Genre::Genre(const QString &name, int ID3v1) :
    m_name(name),
    m_ID3v1(ID3v1)
{

}

////////////////////////////////////////////////////////////////////////////////
// related functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const Genre &g)
{
    s << g.name() << g.ID3v1();
    return s;
}

QDataStream &operator>>(QDataStream &s, Genre &g)
{
    QString name;
    int n;

    s >> name >> n;

    g.setName(name);
    g.setID3v1(n);

    return s;
}
