/**
 * Copyright (C) 2004 Nathan Toone <nathan@toonetown.com>
 * Copyright (C) 2008 Michael Pyne <mpyne@kde.org>
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

#ifndef COVERINFO_H
#define COVERINFO_H

#include "filehandle.h"

#include <QImage>

class QPixmap;

class CoverInfo
{
    friend class FileHandle;

public:
    enum CoverSize { FullSize, Thumbnail };

    explicit CoverInfo(const FileHandle &file);

    bool hasCover() const;

    void clearCover();
    void setCover(const QImage &image = QImage());

    QPixmap pixmap(CoverSize size) const;

    /**
     * Returns the path to the cover data. For embedded covers the art will be
     * extracted to a temporary file, and the returned path will be that of the
     * temporary file. The temporary file will be owned by the caller.
     *
     * Note that it is possible to have a valid filename even for covers that
     * have no embedded art since JuK supports using cover.{jpg,png} in a
     * directory.
     *
     * @param fallbackFileName The filename (including full absolute path)
     * of the file to write to if embedded album art is present and to be
     * extracted.
     *
     * If no cover is present, an empty string is returned.
     */
    QString localPathToCover(const QString &fallbackFileName) const;

    void popup() const;

private:
    QImage scaleCoverToThumbnail(const QImage &image) const;

    // Not supported for all file types as we must build on top of TagLib
    // support.
    QImage embeddedAlbumArt() const;

    bool hasEmbeddedAlbumArt() const;

    FileHandle m_file;

    // Mutable to allow this info to be cached.
    mutable bool m_hasCover = false;
    mutable bool m_hasAttachedCover = false;
    mutable bool m_haveCheckedForCover = false;
};

#endif

// vim: set et sw=4 tw=0 sta:
