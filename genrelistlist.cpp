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

// There are some nasty hacks here to get around not being able to call locate()
// in the initialization of a static data member.  As a result, the function 
// below can not be called in the initialization of another static member and
// if it is, well, all hell will break loose and you'll get a strange segfault.

// public
GenreList *GenreListList::id3v1List()
{
  if(!id3v1Loaded) {
    id3v1->load(locate("data", "juk/id3v1.genreml"));
    id3v1Loaded = true;
  }
  return(id3v1);
}

// private
GenreList *GenreListList::id3v1 = new GenreList(true);
bool GenreListList::id3v1Loaded = false;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

GenreListList::GenreListList()
{
}

GenreListList::~GenreListList()
{
}
