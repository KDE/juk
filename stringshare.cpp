/***************************************************************************
    begin                : Sat Oct 25 2003
    copyright            : (C) 2003 by Maksim Orlovich
    email                : maksim.orlovich@kdemail.net    
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
#include "stringhash.h"

const int SIZE = 5003;

StringShare::Data* StringShare::s_data = 0;

/**
 * We store the strings in two simple direct-mapped (i.e. no collision handling,
 * just replace) hashes, which contain strings or null objects. This costs only
 * 4 bytes per slot on 32-bit archs, so with the default constant size we only
 * really use 40K or so.
 *
 * The end result is that many strings end up pointing to the same underlying data
 * object, instead of each one having its own little copy. 
 */

struct StringShare::Data
{
    QString  qstringHash [SIZE];
    QCString qcstringHash[SIZE];
};

StringShare::Data* StringShare::data()
{
    if (!s_data)
        s_data = new Data;
    return s_data;        
}

QString StringShare::tryShare(const QString& in)
{
    int index = hashString(in) % SIZE;
    
    Data* dat = data();
    if (dat->qstringHash[index] == in) //Match
        return dat->qstringHash[index];
    else
    {
        //Else replace whatever was there before
        dat->qstringHash[index] = in;
        return in;
    }    
}

QCString StringShare::tryShare(const QCString& in)
{
    int index = hashString(in) % SIZE;
 
    Data* dat = data();
    if (dat->qcstringHash[index] == in) //Match
        return dat->qcstringHash[index];
    else
    {
        //Else replace whatever was there before
        dat->qcstringHash[index] = in;
        return in;
    }        
}
