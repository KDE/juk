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

#include <qfileinfo.h>

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

    mutable Tag *tag;
    QFileInfo fileInfo;
    mutable QString absFilePath;
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

FileHandle::FileHandle(const QFileInfo &info, const QString &path)
{
    d = new FileHandlePrivate;
    d->fileInfo = info;
    d->absFilePath = path.isNull() ? info.absFilePath() : path;
}

FileHandle::FileHandle(const QString &path)
{
    d = new FileHandlePrivate;
    d->absFilePath = path;
    d->fileInfo.setFile(path);
}

FileHandle::~FileHandle()
{
    if(d->deref())
        delete d;
}

void FileHandle::refresh()
{
    d->fileInfo.refresh();
    delete d->tag;
    d->tag = Tag::createTag(d->absFilePath);
}

Tag *FileHandle::tag() const
{
    if(!d->tag)
        d->tag = Tag::createTag(d->absFilePath);

    return d->tag;
}

QString FileHandle::absFilePath() const
{
    if(d->absFilePath.isNull())
        d->absFilePath = d->fileInfo.absFilePath();
    return d->absFilePath;
}

const QFileInfo &FileHandle::fileInfo() const
{
    return d->fileInfo;
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
