/***************************************************************************
    begin                : Sun Feb 2 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
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
