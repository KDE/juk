/***************************************************************************
                          filehandle.cpp
                             -------------------
    begin                : Sun Feb 29 2004
    copyright            : (C) 2004 by Scott Wheeler
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

#include "filehandle.h"
#include "tag.h"

/**
 * A simple reference counter -- pasted from TagLib.
 */

class RefCounter
{
public:
    RefCounter() : refCount(1) {}
    void ref() { refCount++; }
    bool deref() { return ! --refCount ; }
    int count() const { return refCount; }
private:
    uint refCount;
};

class FileHandle::FileHandlePrivate : public RefCounter
{
public:
    FileHandlePrivate() :
        tag(0) {}

    Tag *tag;
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

FileHandle::FileHandle()
{
    d = new FileHandlePrivate;
}

FileHandle::FileHandle(const FileHandle &f) : d(f.d)
{
    d->ref();
}

FileHandle::~FileHandle()
{
    if(d->deref())
        delete d;
}

Tag *FileHandle::tag() const
{
    return d->tag;
}

FileHandle &FileHandle::operator=(const FileHandle &f)
{
    if(&f == this)
        return *this;

    if(d->deref())
        delete d;

    d = f.d;
    d->ref();

    return *this;
}

bool FileHandle::operator==(const FileHandle &f) const
{
    return d == f.d;
}
