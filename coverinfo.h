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
#include <qfile.h>

#include "tag.h"

class CoverInfo
{
    friend class FileHandle;

public:
    CoverInfo(const Tag &tag);

    QPixmap *coverPixmap() const;
    bool hasCover() const;
    QPixmap *pixmap(bool large) const;
    QPixmap *largeCoverPixmap() const;
    QString coverLocation(bool large) const;
    void popupLargeCover();


private:
    class CoverPopupWindow;
    friend class CoverPopupWindow;

    Tag m_tag;
    CoverPopupWindow *m_popupWindow;
};
#endif

