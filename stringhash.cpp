/***************************************************************************
                          stringhash.cpp  -  description
                             -------------------
    begin                : Sun Feb 2 2003
    copyright            : (C) 2003 by Scott Wheeler
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

#include "stringhash.h"

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

int StringHash::hash(QString key) const
{
    uint h = 0;
    uint g;

    const QChar *p = key.unicode();

    for(int i = 0; i < int(key.length()); i++) {
        h = (h << 4) + p[i].cell();
        if((g = h & 0xf0000000))
            h ^= g >> 24;
        h &= ~g;
    }

    int index = h;

    if(index < 0)
        index = -index;

    return index % maxSize();
}
