/***************************************************************************
                          genrelistlist.h  -  description
                             -------------------
    begin                : Sun Mar 3 2002
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

#ifndef GENRELISTLIST_H
#define GENRELISTLIST_H

#include <qvaluelist.h>

#include "genrelist.h"

class GenreListList : public QValueList<GenreList>
{
public:
    GenreListList();
    ~GenreListList();

    static GenreList *id3v1List();

private:
    static GenreList *id3v1;
    static bool id3v1Loaded;
};

#endif
