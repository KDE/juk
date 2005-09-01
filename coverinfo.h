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
//Added by qt3to4:
#include <QPixmap>

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

