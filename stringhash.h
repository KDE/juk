/**
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
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

#ifndef STRINGHASH_H
#define STRINGHASH_H

#include <QSet>

/**
 * A simple hash representing an (un-mapped) set of data.
 */
template <class T> class Hash : public QSet<T>
{
public:
    /**
     * To combine two operations into one (that takes the same amount as each
     * independantly) this inserts an item and returns true if the item was
     * already in the set or false if it did not.
     */
    inline bool insert(const T &value)
    {
        if(this->contains(value))
            return true;
        QSet<T>::insert(value);
        return false;
    }
};

typedef Hash<QString> StringHash;

#endif

// vim: set et sw=4 tw=0 sta:
