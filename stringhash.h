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

#include "filehandle.h"

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
        if(contains(value))
            return true;
        QSet<T>::insert(value);
        return false;
    }
};

typedef Hash<QString> StringHash;
typedef Hash<void *> PtrHash;

// cannot be a Hash<FileHandle> because it needs "FileHandle value(QString)"
class FileHandleHash : public QHash<QString, FileHandle>
{
public:
    inline bool insert(const FileHandle &value)
    {
        if(contains(value))
            return true;
        QHash<QString, FileHandle>::insert(value.absFilePath(), value);
        return false;
    }
    inline bool contains(const FileHandle &value) { return QHash<QString, FileHandle>::contains(value.absFilePath()); }
    inline bool remove(const FileHandle &value) { return QHash<QString, FileHandle>::remove(value.absFilePath()); }
};

#endif

// vim: set et sw=4 tw=0 sta:
