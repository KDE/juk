/***************************************************************************
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
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

    QPixmap *coverPixmap();
    bool hasCover() { return (QFile(coverLocation(false)).exists() || QFile(coverLocation(true)).exists()); }
    QPixmap *getPixmap(bool large);
    QPixmap *largeCoverPixmap();
    QString coverLocation(bool large);
    void popupLargeCover();


private:
    Tag m_tag;
};
#endif

