/***************************************************************************
                          genrelistreader.cpp  -  description
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

#include <kdebug.h>

#include "genrelistreader.h"
#include "genrelist.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

GenreListReader::GenreListReader(GenreList &genreList) : 
    QXmlDefaultHandler(),
    m_list(genreList), m_inGenreTag(false)
{

}

GenreListReader::~GenreListReader()
{

}

bool GenreListReader::startElement(const QString &, const QString &,
				   const QString &element,
				   const QXmlAttributes &attributes)
{
    if(element.lower() == "genre") {
        m_inGenreTag = true;
        if(attributes.index("id3v1") != -1)
            m_ID3v1 = attributes.value("id3v1").toInt();
        else
            m_ID3v1 = 255;
    }
    else if(element.lower() == "genrelist") {
	if(attributes.index("name") != -1)
	    m_list.setName(attributes.value("name"));
    }
    else
        m_ID3v1 = 255;

    return true;
}

bool GenreListReader::endElement(const QString &, const QString &, const QString & element)
{
    if(element.lower() == "genre")
        m_inGenreTag = false;

    return true;
}

bool GenreListReader::characters(const QString &content)
{
    if(m_inGenreTag)
        m_list.append(Genre(content, m_ID3v1));

    return true;
}
