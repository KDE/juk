/***************************************************************************
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

#include <limits.h>
#include <stdlib.h>

#include <kdebug.h>

#include <qfileinfo.h>

#include "filehandle.h"
#include "filehandleproperties.h"
#include "tag.h"
#include "cache.h"
#include "coverinfo.h"

AddProperty(Title, tag()->title())
AddProperty(Artist, tag()->artist())
AddProperty(Album, tag()->album())
AddProperty(Genre, tag()->genre())
AddNumberProperty(Track, tag()->track())
AddNumberProperty(Year, tag()->year())
AddProperty(Comment, tag()->comment())
AddNumberProperty(Seconds, tag()->seconds())
AddNumberProperty(Bitrate, tag()->bitrate())
AddProperty(Path, absFilePath())
AddNumberProperty(Size, fileInfo().size())
AddProperty(Extension, fileInfo().extension(false))

static QString resolveSymLinks(const QFileInfo &file) // static
{
    char real[PATH_MAX];

    if(file.exists() && realpath(QFile::encodeName(file.absFilePath()).data(), real))
        return QFile::decodeName(real);
    else
        return file.filePath();
}

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
        tag(0),
        coverInfo(0) {}

    ~FileHandlePrivate()
    {
        delete tag;
        delete coverInfo;
    }

    mutable Tag *tag;
    mutable CoverInfo *coverInfo;
    mutable QString absFilePath;
    QFileInfo fileInfo;
    QDateTime modificationTime;
    QDateTime lastModified;
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

FileHandle::FileHandle()
{
    static FileHandlePrivate nullPrivate;
    d = &nullPrivate;
    d->ref();
}

FileHandle::FileHandle(const FileHandle &f) :
    d(f.d)
{
    if(!d) {
	kdDebug(65432) << "The source FileHandle was not initialized." << endl;
	d = null().d;
    }
    d->ref();
}

FileHandle::FileHandle(const QFileInfo &info, const QString &path) :
    d(0)
{
    setup(info, path);
}

FileHandle::FileHandle(const QString &path) :
    d(0)
{
    setup(QFileInfo(path), path);
}

FileHandle::FileHandle(const QString &path, CacheDataStream &s)
{
    d = new FileHandlePrivate;
    d->fileInfo = QFileInfo(path);
    d->absFilePath = path;
    read(s);
    Cache::instance()->insert(*this);
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
    d->tag = new Tag(d->absFilePath);
}

void FileHandle::setFile(const QString &path)
{
    if(!d || isNull())
        setup(QFileInfo(path), path);
    else {
        d->absFilePath = resolveSymLinks(path);
        d->fileInfo.setFile(path);
        d->tag->setFileName(d->absFilePath);
    }
}

Tag *FileHandle::tag() const
{
    if(!d->tag)
        d->tag = new Tag(d->absFilePath);

    return d->tag;
}

CoverInfo *FileHandle::coverInfo() const
{
    if(!d->coverInfo)
        d->coverInfo = new CoverInfo(*this);

    return d->coverInfo;
}

QString FileHandle::absFilePath() const
{
    if(d->absFilePath.isNull())
        d->absFilePath = resolveSymLinks(d->fileInfo.absFilePath());
    return d->absFilePath;
}

const QFileInfo &FileHandle::fileInfo() const
{
    return d->fileInfo;
}

bool FileHandle::isNull() const
{
    return *this == null();
}

bool FileHandle::current() const
{
    return (d->modificationTime.isValid() &&
            lastModified().isValid() &&
            d->modificationTime >= lastModified());
}

const QDateTime &FileHandle::lastModified() const
{
    if(d->lastModified.isNull())
        d->lastModified = d->fileInfo.lastModified();

    return d->lastModified;
}

void FileHandle::read(CacheDataStream &s)
{
    switch(s.cacheVersion()) {
    case 1:
    default:
        if(!d->tag)
            d->tag = new Tag(d->absFilePath, true);

        s >> *(d->tag);
        s >> d->modificationTime;
        break;
    }
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

bool FileHandle::operator!=(const FileHandle &f) const
{
    return d != f.d;
}

QStringList FileHandle::properties() // static
{
    return FileHandleProperties::properties();
}

QString FileHandle::property(const QString &name) const
{
    return FileHandleProperties::property(*this, name.latin1());
}

const FileHandle &FileHandle::null() // static
{
    static FileHandle f;
    return f;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void FileHandle::setup(const QFileInfo &info, const QString &path)
{
    if(d && !isNull())
        return;

    QString fileName = path.isNull() ? info.absFilePath() : path;

    FileHandle cached = Cache::instance()->value(resolveSymLinks(fileName));

    if(cached != null()) {
        d = cached.d;
        d->ref();
    }
    else {
        d = new FileHandlePrivate;
        d->fileInfo = info;
        d->absFilePath = resolveSymLinks(fileName);
        d->modificationTime = info.lastModified();
        Cache::instance()->insert(*this);
    }
}

////////////////////////////////////////////////////////////////////////////////
// related functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const FileHandle &f)
{
    s << *(f.tag())
      << f.lastModified();

    return s;
}

CacheDataStream &operator>>(CacheDataStream &s, FileHandle &f)
{
    f.read(s);
    return s;
}
