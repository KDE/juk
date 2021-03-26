/**
 * Copyright (C) 2018 Michael Pyne <mpyne@kde.org>
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

#include "directoryloader.h"

#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

#include "mediafiles.h"
#include "collectionlist.h"

// Classifies files into types for potential loading purposes.
enum class MediaFileType {
    UnusableFile,
    MediaFile,
    Playlist,
    Directory
};

static MediaFileType classifyFile(const QFileInfo &fileInfo);
static FileHandle loadMediaFile(const QString &fileName);

DirectoryLoader::DirectoryLoader(const QString &dir, QObject *parent)
    : QObject(parent)
    , m_dirIterator(
        dir,
        QDir::AllEntries | QDir::NoDotAndDotDot,
        QDirIterator::Subdirectories | QDirIterator::FollowSymlinks)
{
}

void DirectoryLoader::startLoading()
{
    static const int BATCH_SIZE = 256;
    FileHandleList files;

    while(m_dirIterator.hasNext()) {
        const auto fileName = m_dirIterator.next();
        const QFileInfo fileInfo(fileName);
        const auto type = classifyFile(fileInfo);

        switch(type) {
            case MediaFileType::Playlist:
                emit loadedPlaylist(fileName);
                break;

            case MediaFileType::MediaFile:
                {
                    const auto loadedMetadata = loadMediaFile(fileInfo.canonicalFilePath());
                    files << loadedMetadata;

                    if(files.count() >= BATCH_SIZE) {
                        emit loadedFiles(files);
                        files.clear();
                    }
                }
                break;

            case MediaFileType::Directory:
                // this should be impossible based on the
                // QDirIterator settings
                continue;
            case MediaFileType::UnusableFile:
                continue;
            default:
                continue;
        }
    }

    if(!files.isEmpty()) {
        emit loadedFiles(files);
    }
}

MediaFileType classifyFile(const QFileInfo &fileInfo)
{
    const QString path = fileInfo.canonicalFilePath();

    if(fileInfo.isFile() && fileInfo.isReadable() &&
        MediaFiles::isMediaFile(path))
    {
        return MediaFileType::MediaFile;
    }

    // These are all the files we care about, anything remaining needs to be a
    // directory or playlist or it must be unusable

    if(MediaFiles::isPlaylistFile(path)) {
        return MediaFileType::Playlist;
    }

    if(fileInfo.isDir()) {
        return MediaFileType::Directory;
    }

    return MediaFileType::UnusableFile;
}

FileHandle loadMediaFile(const QString &fileName)
{
    // Just because our GUI thread accesses are serialized by signal/slot magic
    // doesn't mean that our non-GUI threads don't require deconfliction
    // Neither FileHandle nor TagLib are not thread-safe so synchronize access
    // to this function.
    static QMutex fhLock;
    QMutexLocker locker(&fhLock);

    const auto item = CollectionList::instance()->lookup(fileName);
    if(item) {
        // Don't re-load a file if it's already loaded once
        return FileHandle(item->file());
    }
    else {
        FileHandle loadedMetadata(fileName);
        (void) loadedMetadata.tag(); // Ensure tag is read

        return loadedMetadata;
    }
}
