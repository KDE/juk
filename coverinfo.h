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

class CoverInfo
{
    friend class FileHandle;

public:
    enum CoverSize { FullSize, Thumbnail };

    CoverInfo(const FileHandle &file);

    bool hasCover();

    void clearCover();
    void setCover(const QImage &image = QImage());

    QPixmap pixmap(CoverSize size) const;
    void popup() const;

private:
    QString coverLocation(CoverSize size) const;

    FileHandle m_file;
    bool m_hasCover;
    bool m_haveCheckedForCover;
};
#endif

