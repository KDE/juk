/***************************************************************************
                          genrelist.cpp  -  description
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

#include <kdebug.h>

#include "genrelist.h"
#include "genrelistreader.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

GenreList::GenreList(bool createIndex = false)
{
  hasIndex = createIndex;
}

GenreList::GenreList(QString file, bool createIndex = false)
{
  hasIndex = createIndex;
  load(file);
}

GenreList::~GenreList()
{
}

void GenreList::load(QString file)
{
  GenreListReader *handler = new GenreListReader(this);
  QFile input(file);
  QXmlInputSource source(input);
  QXmlSimpleReader reader;
  reader.setContentHandler(handler);
  reader.parse(source);

  if(hasIndex)
    initializeIndex();  
}

QString GenreList::name(int id3v1)
{
  if(hasIndex && id3v1 >= 0 && id3v1 < int(index.size()))
    return(index[id3v1]);
  else
    return(QString::null);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void GenreList::initializeIndex()
{
  kdDebug() << "initializeIndex()" << endl;
  index.clear();
  index.resize(count());
  for(GenreList::Iterator it = begin(); it != end(); ++it) {
    index[(*it).getId3v1()] = (*it).getName();
  }
}
