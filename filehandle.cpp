/**
 * Copyright (C) 2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2017 Michael Pyne  <mpyne@kde.org>
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

#include "filehandle.h"

#include <QFileInfo>
#include <QSharedData>
#include <QScopedPointer>

#include "filehandleproperties.h"
#include "tag.h"
#include "cache.h"
#include "coverinfo.h"
#include "juk_debug.h"

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
AddProperty(Extension, fileInfo().suffix())

class FileHandle::FileHandlePrivate : public QSharedData
{
public:
    FileHandlePrivate(QFileInfo fInfo)
        : tag(nullptr)
        , coverInfo(nullptr)
        , fileInfo(fInfo)
        , absFilePath(fInfo.canonicalFilePath())
    {
        baseModificationTime = fileInfo.lastModified();
    }

    mutable QScopedPointer<Tag> tag;
    mutable QScopedPointer<CoverInfo> coverInfo;
    QFileInfo fileInfo;
    QString absFilePath;
    QDateTime baseModificationTime;
    mutable QDateTime lastModified;
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

FileHandle::FileHandle(const FileHandle &f) :
    d(f.d)
{
}

FileHandle::FileHandle(const QFileInfo &info) :
    d(new FileHandlePrivate(info))
{
}

FileHandle::FileHandle()
    : FileHandle(QFileInfo()) // delegating ctor
{
}

FileHandle::FileHandle(const QString &path)
    : FileHandle(QFileInfo(path)) // delegating ctor
{
}

FileHandle::FileHandle(const QString &path, CacheDataStream &s)
    : FileHandle(QFileInfo(path)) // delegating ctor
{
    if(d->fileInfo.exists())
        read(s);
}

FileHandle::~FileHandle() = default;

void FileHandle::refresh()
{
    d->fileInfo.refresh();
    d->tag.reset(new Tag(d->absFilePath));
}

void FileHandle::setFile(const QString &path)
{
    if(path.isEmpty()) {
        qCCritical(JUK_LOG) << "trying to set an empty path";
        return;
    }

    if(!QFile::exists(path)) {
        qCCritical(JUK_LOG) << "trying to set non-existent file: " << path;
        return;
    }

    d = new FileHandlePrivate(QFileInfo(path));
}

Tag *FileHandle::tag() const
{
    if(Q_UNLIKELY(!d->tag))
        d->tag.reset(new Tag(d->absFilePath));

    return d->tag.data();
}

CoverInfo *FileHandle::coverInfo() const
{
    if(Q_UNLIKELY(!d->coverInfo))
        d->coverInfo.reset(new CoverInfo(*this));

    return d->coverInfo.data();
}

QString FileHandle::absFilePath() const
{
    return d->absFilePath;
}

const QFileInfo &FileHandle::fileInfo() const
{
    return d->fileInfo;
}

bool FileHandle::isNull() const
{
    return d->absFilePath.isEmpty();
}

bool FileHandle::current() const
{
    return (d->baseModificationTime.isValid() &&
            lastModified().isValid() &&
            d->baseModificationTime >= lastModified());
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
            d->tag.reset(new Tag(d->absFilePath, true));

        s >> *(d->tag);
        s >> d->baseModificationTime;
        break;
    }
}

FileHandle &FileHandle::operator=(const FileHandle &f)
{
    if(&f != this)
        d = f.d;

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
    return FileHandleProperties::property(*this, name.toUtf8());
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

// vim: set et sw=4 tw=0 sta:
