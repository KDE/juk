/***************************************************************************
                          genre.h  -  description
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

#ifndef GENRE_H
#define GENRE_H

#include <qstring.h>
#include <qdatastream.h>

class Genre : public QString
{
public:
    Genre();
    Genre(const QString &genreName, int ID3v1Number = 255);
    Genre &operator=(const QString &);
    
    int getID3v1() const;
    void setID3v1(int number);
    
private:
    QString name;
    int ID3v1;
};

QDataStream &operator<<(QDataStream &s, const Genre &g);
QDataStream &operator>>(QDataStream &s, Genre &g);

#endif
