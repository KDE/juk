/***************************************************************************
    copyright            : (C) 2004 Nathan Toone
    email                : nathan@toonetown.com

    copyright            : (C) 2008 Michael Pyne
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef COVERINFO_H
#define COVERINFO_H

#include "filehandle.h"
#include "covermanager.h"

#include <QImage>

class QPixmap;

class CoverInfo
{
    friend class FileHandle;

public:
    enum CoverSize { FullSize, Thumbnail };

    CoverInfo(const FileHandle &file);

    bool hasCover() const;

    void clearCover();
    void setCover(const QImage &image = QImage());

    // Use this to assign to a specific cover id.
    void setCoverId(coverKey id);

    /**
     * This function sets the cover identifier for all tracks that have the
     * same Artist and Album as this track, to the cover identifier of this
     * track.
     *
     * @param overwriteExistingCovers If set to true, this function will always
     *        apply the new cover to a track even if the track already had
     *        a different cover set.
     */
    void applyCoverToWholeAlbum(bool overwriteExistingCovers = false) const;

    coverKey coverId() const;

    QPixmap pixmap(CoverSize size) const;

    /**
     * Returns the path to the cover data. For embedded covers the art will be
     * extracted to a temporary file, and the returned path will be that of the
     * temporary file. The temporary file will be owned by the caller.
     *
     * Note that it is possible to have a valid filename even for covers that
     * do not have "coverKey" since JuK supports using cover.{jpg,png} in a
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
    mutable bool m_hasCover;
    mutable bool m_hasAttachedCover;
    mutable bool m_haveCheckedForCover;
    mutable coverKey m_coverKey;
};

#endif

// vim: set et sw=4 tw=0 sta:
