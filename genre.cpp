/***************************************************************************
                      genre.cpp  -  description
                             -------------------
    begin                : Tue Feb 5 2002
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

#include "genre.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Genre::Genre() : QString()
{
  name = QString::null;
  id3v1 = 255;
}

Genre::Genre(QString genreName, int id3v1Number) : QString(genreName)
{
  id3v1 = id3v1Number;
}

Genre &Genre::operator=(const QString &genreName)
{
  Genre genre(genreName, this->getId3v1());
  *this = genre;
  return(*this);
}

Genre &Genre::operator=(const char *genreName)
{
  Genre genre(genreName, this->getId3v1());
  *this = genre;
  return(*this);
}

int Genre::getId3v1()
{
  return(id3v1);
}

void Genre::setId3v1(int id3v1Number)
{
  id3v1 = id3v1Number;
}
