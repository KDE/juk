/***************************************************************************
                          genrelistlist.cpp  -  description
                             -------------------
    begin                : Sun Mar 3 2002
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

#include <kstandarddirs.h>
#include <kdebug.h>

#include <qdir.h>

#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

// There are some nasty hacks here to get around not being able to call locate()
// in the initialization of a static data member.  As a result, the function
// below can not be called in the initialization of another static member and
// if it is, well, all hell will break loose and you'll get a strange segfault.

// public

GenreList GenreListList::ID3v1List()
{
    if(!ID3v1) {
	ID3v1 = new GenreList(true);
        ID3v1->load(locate("data", "juk/id3v1.genreml"));
	ID3v1->setReadOnly(true);
    }
    return(*ID3v1);
}

GenreListList GenreListList::lists()
{
    GenreListList l;
    if(ID3v1)
	l.append(*ID3v1);

    QDir dir(KGlobal::dirs()->saveLocation("appdata"));
    
    QStringList files = dir.entryList("*.genreml");
    
    for(QStringList::Iterator it = files.begin(); it != files.end(); it++) {
	GenreList genreList(*it);
	if(genreList.count() > 0)
	    l.append(genreList);
    }

    return(l);
}

// private

GenreList *GenreListList::ID3v1 = 0;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

GenreListList::GenreListList()
{
    
}

GenreListList::~GenreListList()
{

}
