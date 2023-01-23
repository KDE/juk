/**
 * Copyright (C) 2003 Maksim Orlovich <maksim.orlovich@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "stringshare.h"

#include <QHash>

const int SIZE = 8191;

static unsigned num_attempts = 0;
static unsigned num_hits     = 0;

/**
 * We store the strings in a simple direct-mapped (i.e. no collision handling,
 * just replace) hash, which contain strings or null objects. This costs only
 * 4 bytes per slot on 32-bit archs, so with the default constant size we only
 * really use 40K or so.
 *
 * The end result is that many strings end up pointing to the same underlying data
 * object, instead of each one having its own little copy.
 *
 * More importantly, the way the tryShare function is coded ensures that
 * most-recently inserted text stays in the cache, which gives a better chance
 * of continuing to share data. (Even if something old ("foo") that was shared
 * gets kicked out, all the other "foo"s will still be sharing each other's
 * data.
 */

struct StringShare::Data
{
    QString  qstringHash [SIZE];
};

StringShare::Data* StringShare::data()
{
    static Data *data = new Data;
    return data;
}

QString StringShare::tryShare(const QString& in)
{
    uint index = qHash(in) % SIZE;

    num_attempts++;

    Data* dat = data();
    if (dat->qstringHash[index] == in) {
        // Match
        num_hits++;
        return dat->qstringHash[index];
    } else {
        // Else replace whatever was there before
        dat->qstringHash[index] = in;
        return in;
    }
}

unsigned StringShare::numHits()
{
    return num_hits;
}

unsigned StringShare::numAttempts()
{
    return num_attempts;
}

// vim: set et sw=4 tw=0 sta:
