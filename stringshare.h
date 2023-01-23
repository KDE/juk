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

#ifndef STRING_SHARE_H
#define STRING_SHARE_H

class QString;

/**
 * This class attempts to normalize repeated occurrences of strings to use
 *the same shared object, if possible, by using a small hash
 */
class StringShare
{
    struct Data;
public:
    static QString tryShare(const QString& in);
    static unsigned numHits();
    static unsigned numAttempts();

private:
    static Data* data();
};

#endif

// vim: set et sw=4 tw=0 sta:
