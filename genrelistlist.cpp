/***************************************************************************
                          genrelistlist.cpp  -  description
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

#include <kstandarddirs.h>

#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

// public
GenreList GenreListList::id3v1List()
{
  return(id3v1);
}

// private
GenreList GenreListList::id3v1 = GenreList(locate("data", "juk/id3v1.genreml"), true);

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

GenreListList::GenreListList()
{
}

GenreListList::~GenreListList()
{
}
