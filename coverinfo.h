/***************************************************************************
    copyright            : (C) 2004 Nathan Toone
    email                : nathan@toonetown.com
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

#include <qimage.h>

#include "filehandle.h"
#include "covermanager.h"

class CoverInfo
{
    friend class FileHandle;

public:
    enum CoverSize { FullSize, Thumbnail };

    CoverInfo(const FileHandle &file);

    bool hasCover();

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

    coverKey coverId() const { return m_coverKey; }

    QPixmap pixmap(CoverSize size) const;
    void popup() const;

private:
    QString coverLocation(CoverSize size) const;
    bool convertOldStyleCover() const;

    FileHandle m_file;
    bool m_hasCover;
    bool m_haveCheckedForCover;
    mutable coverKey m_coverKey;
    mutable bool m_needsConverting;
};
#endif

