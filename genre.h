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

#ifndef PLAYER_H
#define PLAYER_H

#include <qstring.h>

#define NUM_GENRES 147

class Genre
{
public:
  static QString getGenreName(int index);
  static const int getGenreCount() { return NUM_GENRES; }  
  
private:
  static const QString genreTable[NUM_GENRES + 1];
};

#endif
