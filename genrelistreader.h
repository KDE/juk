/***************************************************************************
                          genrelistreader.h  -  description
                             -------------------
    begin                : Mon Mar 4 2002
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

#ifndef GENRELISTREADER_H
#define GENRELISTREADER_H

#include <qxml.h>

#include "genrelist.h"

/**
 * A SAX2 based XML reader to read GenreLists.  Since there's little use to keep
 * the document structure in memory, I've opted for SAX2 over QDom, since it's 
 * a good deal faster.
 */ 

class GenreListReader : public QXmlDefaultHandler
{
public:
    GenreListReader(GenreList *genreList);
    virtual ~GenreListReader();

    bool startElement(const QString &, const QString &, const QString & element, const QXmlAttributes & attributes);
    bool endElement(const QString &, const QString &, const QString &);

    bool characters(const QString &content);

private:
    GenreList *list;
    bool inGenreTag;
    int ID3v1;
};

#endif
