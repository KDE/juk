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


#include <qpixmap.h>

#include "filehandle.h"

class CoverInfo
{
    friend class FileHandle;

public:
    enum CoverSize { FullSize, Thumbnail };

    CoverInfo(const FileHandle &file);

    QPixmap coverPixmap() const;
    bool hasCover();
    void resetHasCover();
    QPixmap pixmap(CoverSize size) const;
    QPixmap largeCoverPixmap() const;
    QString coverLocation(CoverSize size) const;
    void popupLargeCover();

private:
    class CoverPopupWindow;
    friend class CoverPopupWindow;

    FileHandle m_file;
    bool m_hasCover;
    bool m_haveCheckedForCover;
    CoverPopupWindow *m_popupWindow;
};
#endif

