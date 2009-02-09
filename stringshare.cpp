/***************************************************************************
    begin                : Sat Oct 25 2003
    copyright            : (C) 2003 by Maksim Orlovich
    email                : maksim.orlovich@kdemail.net

    copyright            : (C) 2009 by Michael Pyne
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "stringshare.h"

#include <QSet>

const int SIZE = 5003;

StringShare::Data* StringShare::s_data = 0;

/**
 * We store the strings in a simple direct-mapped (i.e. no collision handling,
 * just replace) hash, which contains strings.  We limit the number of entries
 * in the set to SIZE to avoid excessive growth.
 *
 * The end result is that many strings end up pointing to the same underlying data
 * object, instead of each one having its own little copy.
 */

struct StringShare::Data
{
    QSet<QString> qstringHash;
};

StringShare::Data* StringShare::data()
{
    if (!s_data)
        s_data = new Data;
    return s_data;
}

QString StringShare::tryShare(const QString& in)
{
    Data* dat = data();

    QSet<QString>::const_iterator found = dat->qstringHash.constFind(in);
    if(found != dat->qstringHash.constEnd())
        return *found;

    // Insert if we have room and now this one is the standard
    if(dat->qstringHash.size() < SIZE)
        dat->qstringHash.insert(in);
    return in;
}

// vim: set et sw=4 tw=0 sta:
