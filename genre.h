/***************************************************************************
                          genre.h  -  description
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

#ifndef GENRE_H
#define GENRE_H

#include <qstring.h>

class Genre : public QString
{
public:
  Genre();
  Genre(QString genreName, int id3v1Number = 255);

  Genre &operator=(const QString &);
  Genre &operator=(const char *);

  int getId3v1();

  void setId3v1(int number);

private:
  QString name;
  int id3v1;
};

#endif
