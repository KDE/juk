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

GenreList::GenreList(bool createIndex) : QValueList<Genre>()
{
    hasIndex = createIndex;
}

GenreList::GenreList(QString file, bool createIndex) : QValueList<Genre>()
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
    if(hasIndex && id3v1 >= 0 && id3v1 <= int(index.size()))
        return(index[id3v1]);
    else
        return(QString::null);
}

int GenreList::findIndex(QString item)
{

    // cache the previous search -- since there are a lot of "two in a row"
    // searchs this should optimize things a little

    static QString lastItem;
    static int lastIndex;

    if(!lastItem.isEmpty() && lastItem == item) {
        //    kdDebug() << "GenreList::findIndex() -- cache hit" << endl;
        return(lastIndex);
    }

    int i = 0;
    for(GenreList::Iterator it = begin(); it != end(); ++it) {
        if(item == (*it)) {
            lastItem = item;
            lastIndex = i;
            return(i);
        }
        i++;
    }
    return(-1);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void GenreList::initializeIndex()
{
    //  kdDebug() << "initializeIndex()" << endl;
    index.clear();
    //  kdDebug() << "Cleared size: " << index.size() << endl;
    index.resize(count() + 1);
    for(GenreList::Iterator it = begin(); it != end(); ++it) {
        if((*it).getId3v1() >= 0 && (*it).getId3v1() <= int(index.size())) {
            //      kdDebug() << "initializeIndex() - " << (*it).getId3v1()  << " - "
            //                << index.size() << " - " << count() << " - " << (*it) << endl;
            index[(*it).getId3v1()] = QString(*it);
        }
    }
}
